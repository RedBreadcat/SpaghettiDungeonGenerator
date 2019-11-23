#include "BoringAreaRemover.h"
#include "Map.h"
#include <climits>
#include <queue>

extern Map m;

BoringAreaRemover::BoringAreaRemover()
{
	SetSize(m.width, m.height);

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			Set(i, j, INT_MAX);
		}
	}
}

void BoringAreaRemover::Run()
{
	//Calculate distance map
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			TileType type = m.Get(i, j).type;
			if (type == TileType::CorridorDilatedToMaximumPreviouslyWFC || type == TileType::CorridorFloor)
			{
				FloodFillToProduceDistanceMap({ i, j });
			}
		}
	}

	//If distance above threshold value, and is boring, make tile a wall
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			//todo: make IsTileBoring() function
			TileType type = m.Get(i, j).type;
			if (type == TileType::Nothing || type == TileType::CorridorDilatedToMaximum)
			{
				int distance = Get(i, j);
				//INT_MAX is untouched by the algorithm, and so can't really be classified as boring
				//It could be the little nooks and crannies of empty space inside/between rooms, which we would like to keep
				if (distance != INT_MAX && distance > 10)
				{
					m.Set(i, j, Tile(TileType::FreeWall));
				}
			}
		}
	}
}

void BoringAreaRemover::FloodFillToProduceDistanceMap(Coord start)
{
	std::queue<std::pair<Coord, int>> to_update;	//coord, distance
	to_update.push({ start, 0 });

	auto check_neighbour_and_push =
		[&](Coord neighbour, int distance)
	{
		if (IsValid(neighbour))
		{
			TileType type = m.Get(neighbour).type;
			if (type == TileType::Nothing || type == TileType::CorridorDilatedToMaximum)
			{
				if (Get(neighbour) > distance)
				{
					Set(neighbour, distance);
					to_update.push({ neighbour, distance });
				}
			}
		}
	};

	while (!to_update.empty())
	{
		auto [coord, distance] = to_update.front();
		to_update.pop();

		distance++;
		check_neighbour_and_push(coord + Coord{ -1, 0 }, distance);
		check_neighbour_and_push(coord + Coord{ 1, 0 }, distance);
		check_neighbour_and_push(coord + Coord{ 0, -1 }, distance);
		check_neighbour_and_push(coord + Coord{ 0, 1 }, distance);
	}
}