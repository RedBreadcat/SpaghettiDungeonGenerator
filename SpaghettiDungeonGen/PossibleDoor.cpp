#include "PossibleDoor.h"
#include "Map.h"
#include "ConnectionNode.h"
#include "Corridor.h"
#include <algorithm>

extern Map m;

PossibleDoor::PossibleDoor()
{

}

//(x1, y1) must be earlier in the array than (x2, y2)
PossibleDoor::PossibleDoor(int x1, int y1, int x2, int y2, TileWallData& tiles_and_walls, bool open_connectable) :
	open_connectable(open_connectable),
	connection_opt(nullptr),
	tile1_relative{x1, y1},
	tile2_relative{x2, y2}
{
	if (x2 == -1)	//Single-tile
	{
		facing = DoorFacing::SingleTile;

		if (!SetOutsideTile(x1, y1 - 1, tiles_and_walls))
		{
			if (!SetOutsideTile(x1, y1 + 1, tiles_and_walls))
			{
				if (!SetOutsideTile(x1 - 1, y1, tiles_and_walls))
				{
					if (!SetOutsideTile(x1 + 1, y1, tiles_and_walls))
					{
						std::cout << "Door isn't adjacent to outside. Should not happen\n";
						assert(false);
					}
				}
			}
		}
	}
	else if (x1 == x2)	//Vertical
	{
		if (SetOutsideTile(x1, y1 - 1, tiles_and_walls))	//Outside tile is earlier than door in array
		{
			facing = DoorFacing::Top;
		}
		else
		{
			if (SetOutsideTile(x2, y2 + 1, tiles_and_walls))
			{
				facing = DoorFacing::Bottom;
			}
			else
			{
				std::cout << "Door tile found twice. Should not happen\n";
				assert(false);
			}
		}
	}
	else  //Horizontal
	{
		if (SetOutsideTile(x1 - 1, y1, tiles_and_walls))	//Outside tile is earlier than door in array
		{
			facing = DoorFacing::Left;
		}
		else
		{
			if (SetOutsideTile(x2 + 1, y2, tiles_and_walls))
			{
				facing = DoorFacing::Right;
			}
			else
			{
				std::cout << "Door tile found twice. Should not happen\n";
				assert(false);
			}
		}
	}
}

bool PossibleDoor::SetOutsideTile(int x, int y, TileWallData& tiles_and_walls)
{
	bool found = any_of(tiles_and_walls.tiles.begin(), tiles_and_walls.tiles.end(),
		[x, y](auto& point)
	{
		return point.x == x && point.y == y;
	});

	if (!found)	//If it's not already a tile, that means the tile is on the outside of the room
	{
		found = any_of(tiles_and_walls.walls.begin(), tiles_and_walls.walls.end(),
			[x, y](auto& point)
		{
			return point.x == x && point.y == y;
		});

		if (!found)
		{
			tile_outside_relative = { x, y };
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void PossibleDoor::SetConnectionWithCorridor(ConnectionNode& node_this, Corridor& corridor)
{
	connection_opt = &corridor;
	corridor.AddConnection(node_this);
}

void PossibleDoor::SetConnectionWithAnotherDoor(PossibleDoor& door_other, ConnectionNode& node_this, ConnectionNode& node_other)
{
	connection_opt = &node_other;
	door_other.connection_opt = &node_this;
}

bool PossibleDoor::Connected() const
{
	return connection_opt != nullptr;
}

PossibleDoor PossibleDoor::Get90RotatedCopy(int room_height_or_width) const
{
	auto rotate = [room_height_or_width](Coord t) -> Coord
	{
		return { room_height_or_width - t.y - 1, t.x };
	};

	PossibleDoor d;

	d.open_connectable = open_connectable;
	d.tile1_relative = rotate(tile1_relative);
	d.tile_outside_relative = rotate(tile_outside_relative);

	if (facing == DoorFacing::SingleTile)
	{
		d.tile2_relative = { -1, -1 };
		d.facing = DoorFacing::SingleTile;
	}
	else
	{
		d.tile2_relative = rotate(tile2_relative);

		switch (facing)
		{
			case DoorFacing::Top:
				d.facing = DoorFacing::Right;
				std::swap(d.tile1_relative, d.tile2_relative);	//tile 1 must always be earler in the array. When going from top to right, or bottom to left, need to swap to ensure this.
				break;
			case DoorFacing::Right:
				d.facing = DoorFacing::Bottom;
				break;
			case DoorFacing::Bottom:
				d.facing = DoorFacing::Left;
				std::swap(d.tile1_relative, d.tile2_relative);
				break;
			case DoorFacing::Left:
				d.facing = DoorFacing::Top;
				break;
		}
	}

	return d;
}

void PossibleDoor::Place(Coord room, ConnectionNode& node) const
{
	Coord tile1 = room + tile1_relative;
	m.data[tile1.y][tile1.x] = Tile(TileType::PossibleConnection, &node);

	if (facing != DoorFacing::SingleTile)
	{
		Coord tile2 = room + tile2_relative;
		m.data[tile2.y][tile2.x] = Tile(TileType::PossibleConnection, &node);
	}
}

void PossibleDoor::Unplace(Coord room) const
{
	Coord tile1 = room + tile1_relative;
	m.data[tile1.y][tile1.x] = Tile(TileType::Nothing);

	if (facing != DoorFacing::SingleTile)
	{
		Coord tile2 = room + tile2_relative;
		m.data[tile2.y][tile2.x] = Tile(TileType::Nothing);
	}
}

Coord PossibleDoor::GetOutsideAdjacentTile() const
{
	switch (facing)
	{
		case DoorFacing::Top:
			return tile1_relative;
		case DoorFacing::Right:
			return tile2_relative;
		case DoorFacing::Bottom:
			return tile2_relative;
		case DoorFacing::Left:
			return tile1_relative;
		case DoorFacing::SingleTile:
			return tile1_relative;
		default:
			assert(false && "Invalid facing");
			return Coord{ -1, -1 };
	}
}

void PossibleDoor::PlaceAllDoorTiles()
{
	for (auto& room : m.rooms)
	{
		for (auto& d : room.possible_doors)
		{
			if (d.open_connectable)
			{
				if (d.Connected())
				{
					m.Set(room.p + d.tile1_relative, Tile(TileType::OpenConnection, d.connection_opt));
				}
				else
				{
					m.Set(room.p + d.tile1_relative, Tile(TileType::RoomWall, d.connection_opt));
				}
			}
			else
			{
				if (d.Connected())
				{
					m.Set(room.p + d.tile1_relative, Tile(TileType::Door, d.connection_opt));

					if (d.facing != DoorFacing::SingleTile)
					{
						m.Set(room.p + d.tile2_relative, Tile(TileType::Door, d.connection_opt));
					}
				}
				else
				{
					m.Set(room.p + d.tile1_relative, Tile(TileType::RoomWall, d.connection_opt));

					if (d.facing != DoorFacing::SingleTile)
					{
						m.Set(room.p + d.tile2_relative, Tile(TileType::RoomWall, d.connection_opt));
					}
				}
			}
		}
	}
}