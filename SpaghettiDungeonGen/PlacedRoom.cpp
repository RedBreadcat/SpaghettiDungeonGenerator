#include "PlacedRoom.h"
#include "Map.h"
#include <iostream>

extern Map m;

PlacedRoom::PlacedRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, TileWallData* tile_wall_data, int rotation_in, int prefab_index) : //Prefab room
	p{ x, y },
	rotation(rotation_in),
	prefab_index(prefab_index)
{
	//Doors that face the edge of the map won't be placed, and will just be a wall
	std::vector<Coord> extra_walls;
	for (auto& d : doors_in)
	{
		if (m.IsValid(p + d.tile_outside_relative))
		{
			possible_doors.push_back(d);
		}
		else
		{
			extra_walls.push_back(d.tile1_relative);
			if (d.facing != DoorFacing::SingleTile)
			{
				extra_walls.push_back(d.tile2_relative);
			}
		}
	}

	if (extra_walls.size() == 0)	//No extra walls
	{
		this->tile_wall_data = tile_wall_data;
	}
	else //Added extra walls, so we need to store a custom tile layout in the unique_ptr
	{
		auto custom_data = std::make_unique<TileWallData>();	//Don't use the constructor with arguments, because that moves
		custom_data->tiles = tile_wall_data->tiles;
		custom_data->walls = tile_wall_data->walls;
		custom_data->walls.insert(custom_data->walls.end(), extra_walls.begin(), extra_walls.end());
		this->tile_wall_data = std::move(custom_data);
	}

	Place();
}

PlacedRoom::PlacedRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, std::unique_ptr<TileWallData> tile_wall_data) :	//Proc gen room
	p{ x, y },
	rotation(0),
	prefab_index(-1)
{
	//Doors that face the edge of the map won't be placed, and will just be a wall
	for (auto& d : doors_in)
	{
		if (m.IsValid(p + d.tile_outside_relative))
		{
			possible_doors.push_back(d);
		}
		else
		{
			tile_wall_data->walls.push_back(d.tile1_relative);
			if (d.facing != DoorFacing::SingleTile)
			{
				tile_wall_data->walls.push_back(d.tile2_relative);
			}
		}
	}

	this->tile_wall_data = std::move(tile_wall_data);
	Place();
}

void PlacedRoom::Place()
{
	TileWallData* data;
	if (std::holds_alternative<TileWallData*>(tile_wall_data))
	{
		data = std::get<TileWallData*>(tile_wall_data);
	}
	else
	{
		data = std::get<std::unique_ptr<TileWallData>>(tile_wall_data).get();
	}

	for (auto& w : data->walls)
	{
		m.Set(p + w, Tile(TileType::RoomWall, this));
	}

	for (auto& tile : data->tiles)
	{
		m.Set(p + tile, Tile(TileType::RoomFloor, this));
	}

	for (auto& d : possible_doors)
	{
		d.Place(p, *this);
	}
}

bool PlacedRoom::IsRoom() const
{
	return true;
}

bool PlacedRoom::IsCorridor() const
{
	return false;
}

void PlacedRoom::Unplace() //Not the destructor, because sometimes I may want to delete a room from memory, yet leave the tiles placed still
{
	TileWallData* data;
	if (std::holds_alternative<TileWallData*>(tile_wall_data))
	{
		data = std::get<TileWallData*>(tile_wall_data);
	}
	else
	{
		data = std::get<std::unique_ptr<TileWallData>>(tile_wall_data).get();
	}

	for (auto& c : data->tiles)
	{
		m.Set(p + c, Tile(TileType::Nothing));
	}

	for (auto& c : data->walls)
	{
		m.Set(p + c, Tile(TileType::Nothing));
	}

	for (auto& d : possible_doors)
	{
		d.Unplace(p);
	}
}

void PlacedRoom::ConnectToNearbyRoomsOrCorridors()
{
	for (auto& d : possible_doors)
	{
		if (!d.Connected())
		{
			TryPlaceDoor(d);
		}
	}
}

void PlacedRoom::TryPlaceDoor(PossibleDoor& d)
{
	Coord d1_outside = p + d.tile_outside_relative;

	if (m.IsValid(d1_outside))
	{
		if (m.Get(d1_outside).type == TileType::CorridorWall)
		{
			d.SetConnectionWithCorridor(*this, static_cast<Corridor&>(m.Get(d1_outside).GetNode()));
		}
		else
		{
			DoorFacing desired_other_door_facing_unless_single;
			switch (d.facing)
			{
				case DoorFacing::Top:
					desired_other_door_facing_unless_single = DoorFacing::Bottom;
					break;
				case DoorFacing::Bottom:
					desired_other_door_facing_unless_single = DoorFacing::Top;
					break;
				case DoorFacing::Left:
					desired_other_door_facing_unless_single = DoorFacing::Right;
					break;
				case DoorFacing::Right:
					desired_other_door_facing_unless_single = DoorFacing::Left;
					break;
				case DoorFacing::SingleTile:
					desired_other_door_facing_unless_single = DoorFacing::SingleTile;	//We don't actually desire single tile. We desire any tile.
					break;
			}

			for (auto& other_r : m.rooms)
			{
				if (&other_r != this)	//Don't want to compare a room to itself
				{
					//One of the rooms involved must be normal or procedural. We don't want to connect special rooms to special rooms.
					if (room_type == RoomType::Normal || other_r.room_type == RoomType::Normal || room_type == RoomType::Procedural || other_r.room_type == RoomType::Procedural)
					{
						for (auto& other_d : other_r.possible_doors)
						{
							if (d1_outside == other_r.p + other_d.GetOutsideAdjacentTile())
							{
								d.SetConnectionWithAnotherDoor(other_d, *this, other_r);
							}
						}
					}
				}
			}
		}
	}
}

int PlacedRoom::UnconnectedDoors() const
{
	int unconnected = 0;

	for (const auto& d : possible_doors)
	{
		if (!d.Connected())
		{
			unconnected++;
		}
	}
	return unconnected;
}

bool PlacedRoom::HasConnectionAvailable() const
{
	for (const auto& d : possible_doors)
	{
		if (!d.Connected())
		{
			return true;
		}
	}

	return false;
}