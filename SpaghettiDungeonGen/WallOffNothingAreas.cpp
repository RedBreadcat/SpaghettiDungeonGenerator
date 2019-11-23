#include "WallOffNothingAreas.h"
#include "Map.h"
#include <queue>

extern Map m;

void WallOffNothingAreas::Run()
{
	MapArray<Tile> map_copy = m;

	for (int j = 0; j < m.height; j++)
	{
		for (int i = 0; i < m.width; i++)
		{
			if (map_copy.Get(i, j).type == TileType::Nothing)
			{
				FloodFill({i, j}, map_copy);
			}
		}
	}
}

void WallOffNothingAreas::FloodFill(Coord start, MapArray<Tile>& map_copy)
{
	std::vector<Coord> possible_walls;
	std::queue<Coord> to_update;
	to_update.push(start);

	auto check_neighbour_and_push =
		[&](Coord neighbour)
	{
		if (map_copy.IsValid(neighbour))
		{
			if (map_copy.Get(neighbour).type == TileType::Nothing)
			{
				possible_walls.push_back(neighbour);
				map_copy.Set(neighbour, Tile(TileType::FreeWall));
				to_update.push(neighbour);
			}
		}
	};

	while (!to_update.empty())
	{
		Coord c = to_update.front();
		to_update.pop();

		check_neighbour_and_push(c + Coord{ -1, 0 });
		check_neighbour_and_push(c + Coord{ 1, 0 });
		check_neighbour_and_push(c + Coord{ 0, -1 });
		check_neighbour_and_push(c + Coord{ 0, 1 });
	}

	//If there's a lot of walls we can place, then apply changes to map
	if (possible_walls.size() > 60)
	{
		for (const auto w : possible_walls)
		{
			m.Set(w, Tile(TileType::FreeWall));
		}
	}
}