#include "Corridor.h"
#include "Map.h"

using namespace std;

extern Map m;

Corridor::Corridor(ConnectionNode& start_node, ConnectionNode& end_node, vector<Coord>& tiles_in)
{
	tile_wall_data = make_unique<TileWallData>();
	tile_wall_data->tiles = std::move(tiles_in);
	for (const Coord& p : tile_wall_data->tiles)
	{
		//If it's a corridor tile, then this is a corridor intersection of sorts.
		//This means we need to look through every corridor, checking whether it occupies this tile.
		//Note: we can't just access the node of the tile and check those connections, because then we'd need to check the connections of that node.
		//And the connections of that node would also be the original node. It'd be very recursive and annoying to code.
		if (m.Get(p.x, p.y).type == TileType::CorridorFloor)
		{
			for (auto& other_c : m.corridors)
			{
				if (find_if(connections.begin(), connections.end(), [&](reference_wrapper<ConnectionNode>& con) { return con.get() == other_c; }) == connections.end())	//If we're not connected to the other corridor already
				{
					if (find_if(other_c.tile_wall_data->tiles.begin(), other_c.tile_wall_data->tiles.end(), [&](Coord& coord_compare) { return coord_compare == p; }) != other_c.tile_wall_data->tiles.end())	//If the other corridor has a tile that overlaps with ours
					{
						AddConnection(other_c);
						other_c.AddConnection(*this);
					}
				}
			}
		}
		else
		{
			m.Set(p.x, p.y, Tile(TileType::CorridorFloor, this));
		}
	}

	AddConnection(start_node);
	AddConnection(end_node);
}

optional<reference_wrapper<Corridor>> Corridor::TryToPlace(ConnectionNode& start_node, ConnectionNode& end_node, Coord start, Coord end, int dilation_iterations)
{
	auto path_opt = m.empty_space_pathfinder.GetPath(start, end);
	if (path_opt)
	{
		return TryToPlace(start_node, end_node, *path_opt, dilation_iterations);
	}
	return nullopt;
}

optional<reference_wrapper<Corridor>> Corridor::TryToPlace(ConnectionNode& start_node, ConnectionNode& end_node, const std::vector<Coord>& path_tiles, int dilation_iterations)
{
	if (path_tiles.size() > 50)	//Don't allow corridors that are too long
	{
		return nullopt;
	}

	vector<Coord> all_dilated_tiles, dilated_tiles_previous_iteration;
	size_t failed_dilations = 0;
	for (int dilation = 0; dilation < dilation_iterations; dilation++)
	{
		if (dilation > 0)
		{
			MapArray map_copy = m;

			for (Coord& p : dilated_tiles_previous_iteration)	//Copy dilations from previous iteration into map_copy
			{
				map_copy.data[p.y][p.x].type = TileType::CorridorFloor;
			}

			vector<Coord> dilated_tiles_next_iteration;
			DilateTiles(dilated_tiles_previous_iteration, map_copy, dilated_tiles_next_iteration, failed_dilations);
			dilated_tiles_previous_iteration = dilated_tiles_next_iteration;
		}
		else //Don't need to make a copy when it's the first iteration
		{
			DilateTiles(path_tiles, m, dilated_tiles_previous_iteration, failed_dilations);
		}
		all_dilated_tiles.insert(all_dilated_tiles.end(), dilated_tiles_previous_iteration.begin(), dilated_tiles_previous_iteration.end());	//Add tiles from this iteration
	}

	if (dilation_iterations == 0)
	{
		all_dilated_tiles = path_tiles;
	}

	if (/*failed_dilations < path_tiles.size() * 2 &&*/ all_dilated_tiles.size() > 10)	//If there aren't many failed dilations relative to the length and if the corridor is long enough, place it
	{
		return m.AddCorridor(make_unique<Corridor>(start_node, end_node, all_dilated_tiles));
	}
	else
	{
		return nullopt;
	}
}

void Corridor::DilateTile(int x, int y, MapArray<Tile>& map_data, vector<Coord>& dilated_tiles, size_t& failed_dilations)
{
	CheckTileWhenPlacing(x, y - 1, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x + 1, y, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x, y + 1, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x - 1, y, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x - 1, y - 1, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x + 1, y + 1, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x - 1, y + 1, map_data, dilated_tiles, failed_dilations);
	CheckTileWhenPlacing(x + 1, y - 1, map_data, dilated_tiles, failed_dilations);
}

void Corridor::DilateTiles(const vector<Coord>& tiles_to_dilate, MapArray<Tile>& map_data, vector<Coord>& dilated_tiles, size_t& failed_dilations)
{
	for (const Coord& p : tiles_to_dilate)
	{
		DilateTile(p.x, p.y, map_data, dilated_tiles, failed_dilations);
	}
}

void Corridor::CheckTileWhenPlacing(int x, int y, MapArray<Tile>& map_data, vector<Coord>& dilated_tiles, size_t& failed_dilations)
{
	if (m.IsValid(x, y))
	{
		TileType tile_type = map_data.data[y][x].type;
		if (tile_type == TileType::Nothing || tile_type == TileType::WFCFloor || tile_type == TileType::WFCWall || tile_type == TileType::CorridorWall || tile_type == TileType::CorridorFloor)
		{
			Coord compare = { x, y };
			if (std::find(dilated_tiles.begin(), dilated_tiles.end(), compare) == dilated_tiles.end()) //Verify that we haven't already placed the tile
			{
				dilated_tiles.push_back({ x, y });
			}
		}
		else
		{
			failed_dilations++;
		}
	}
}

void Corridor::CheckTileForWallPlacement(Coord neighbour, vector<Coord>& dilated_tiles)
{
	if (m.IsValid(neighbour))
	{
		const Tile& tile = m.data[neighbour.y][neighbour.x];
		if (tile.type == TileType::Nothing || tile.type == TileType::WFCFloor)	//Don't want to dilate on corridors when in wall-placement mode, otherwise the entire corridor will be made of walls
		{
			if (std::find(dilated_tiles.begin(), dilated_tiles.end(), neighbour) == dilated_tiles.end()) //Verify that we haven't already placed the tile
			{
				dilated_tiles.push_back(neighbour);
			}
		}
	}
}

void Corridor::CheckTileForDilationToMaximum(Coord current, Coord neighbour, vector<Coord>& previously_dilated, vector<Coord>& dilated_tiles, vector<Coord>& dilated_into_walls)
{
	if (m.IsValid(neighbour))
	{
		const Tile& tile = m.data[neighbour.y][neighbour.x];
		if (tile.type == TileType::Nothing || tile.type == TileType::WFCFloor)	//Don't want to dilate on corridors when in wall-placement mode, otherwise the entire corridor will be made of walls
		{
			if (std::find(previously_dilated.begin(), previously_dilated.end(), neighbour) == previously_dilated.end())	//Verify that we haven't already placed the tile
			{
				if (std::find(dilated_tiles.begin(), dilated_tiles.end(), neighbour) == dilated_tiles.end())			//Verify that we haven't already decided to place the tile this iteration
				{
					dilated_tiles.push_back(neighbour);
				}
			}
		}
		else if (tile.type == TileType::CorridorFloor && &tile.GetNode() != this)
		{
			dilated_into_walls.push_back(current);
		}
	}
}

void Corridor::DilateToMaximum()
{
	//The first time we don't want to turn previous tiles into walls, since previous tiles are part of the confirmed floor data the first time,
	//and must remain floors to maintain connectivity and navigability
	bool first_time = true;
	vector<Coord> previous_dilations = tile_wall_data->tiles;
	while (true)
	{
		vector<Coord> current_dilations;
		vector<Coord> dilated_to_walls;

		for (const Coord& p : previous_dilations)
		{
			CheckTileForDilationToMaximum(p, { p.x, p.y - 1 }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x + 1, p.y }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x, p.y + 1 }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x - 1, p.y }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x - 1, p.y - 1 }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x + 1, p.y + 1 }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x - 1, p.y + 1 }, previous_dilations, current_dilations, dilated_to_walls);
			CheckTileForDilationToMaximum(p, { p.x + 1, p.y - 1 }, previous_dilations, current_dilations, dilated_to_walls);
		}

		if (!first_time)
		{
			for (const auto& w : dilated_to_walls)
			{
				m.Set(w, Tile(TileType::CorridorWall, this));
				tile_wall_data->walls.push_back(w);

				std::remove(previous_dilations.begin(), previous_dilations.end(), w);
			}

			for (const auto& p : previous_dilations)
			{
				if (m.Get(p).type == TileType::WFCFloor)
				{
					m.Set(p, Tile(TileType::CorridorDilatedToMaximumPreviouslyWFC, this));
				}
				else
				{
					m.Set(p, Tile(TileType::CorridorDilatedToMaximum, this));
				}
				tile_wall_data->tiles.push_back(p);
			}
		}

		//Dilated to maximum
		if (current_dilations.size() == 0)
		{
			break;
		}

		previous_dilations = current_dilations;
		first_time = false;
	}
}

void Corridor::PlaceWalls()	//Dilate corridor. If a dilation is successful, place a wall.
{
	vector<Coord> dilated_tiles;

	for (const Coord& p : tile_wall_data->tiles)
	{
		CheckTileForWallPlacement({ p.x, p.y - 1 }, dilated_tiles);
		CheckTileForWallPlacement({ p.x + 1, p.y }, dilated_tiles);
		CheckTileForWallPlacement({ p.x, p.y + 1 }, dilated_tiles);
		CheckTileForWallPlacement({ p.x - 1, p.y }, dilated_tiles);
		CheckTileForWallPlacement({ p.x - 1, p.y - 1 }, dilated_tiles);
		CheckTileForWallPlacement({ p.x + 1, p.y + 1 }, dilated_tiles);
		CheckTileForWallPlacement({ p.x - 1, p.y + 1 }, dilated_tiles);
		CheckTileForWallPlacement({ p.x + 1, p.y - 1 }, dilated_tiles);
	}

	for (const auto& p : dilated_tiles) 	//If a tile was successfully dilated, then place a wall there
	{
		m.data[p.y][p.x] = Tile(TileType::CorridorWall, this);
		tile_wall_data->walls.push_back(p);
	}
}

bool Corridor::IsRoom() const
{
	return false;
}

bool Corridor::IsCorridor() const
{
	return true;
}

void Corridor::Unplace()
{
	for (auto& c : tile_wall_data->tiles)
	{
		m.data[c.y][c.x] = Tile();
	}

	//Imagine a corridor that moves through another corridor. The corridors overlap, and so when we un-place we're removing tiles from both corridors
	//By re-placing other corridors, we ensure they remain intact
	for (auto connection : connections)
	{
		if (connection.get().IsCorridor())
		{
			static_cast<Corridor&>(connection.get()).RePlaceMetadata();
		}
	}
}

void Corridor::RePlaceMetadata()
{
	for (auto& c : tile_wall_data->tiles)
	{
		m.data[c.y][c.x] = Tile(TileType::CorridorFloor, this);
	}
}

//Does not verify that connections are double-ended
void Corridor::AddConnection(ConnectionNode& node)
{
	//No duplicates
	auto found = std::find_if(connections.begin(), connections.end(),
		[&](reference_wrapper<ConnectionNode> connection)
	{
		return connection.get() == node;
	});

	if (found == connections.end())
	{
		connections.push_back(node);
	}
}
