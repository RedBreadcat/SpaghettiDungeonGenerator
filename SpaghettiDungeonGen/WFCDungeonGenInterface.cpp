#include "WFCDungeonGenInterface.h"
#include "WaveFunctionCollapse.h"
#include "Map.h"
#include "DRandom.h"
#include <boost/filesystem.hpp>

extern Map m;

namespace WFC
{

WFCDungeonGenInterface::WFCDungeonGenInterface(std::string_view load_disk_path)
{
	std::string patterns_path = std::string(load_disk_path) + "\\Patterns";
	boost::filesystem::directory_iterator end_iter;
	for (boost::filesystem::directory_iterator itr(patterns_path); itr != end_iter; itr++)
	{
		if (boost::filesystem::is_regular_file(itr->path()) && boost::filesystem::extension(itr->path()) == ".json")
		{
			pattern_paths.push_back(itr->path().string());
		}
	}
}

int WFCDungeonGenInterface::Place(int x, int y, int width, int height)
{
	const std::string& path = pattern_paths[DRandom::instance->GetInt(0, pattern_paths.size() - 1)];

	int placed = 0;
	//Could alternatively use the Map to fill up WFC output with pre-collapsed values
	WaveFunctionCollapse wfc(path, width, height);
	auto output = wfc.Run();
	for (int j = y; j < y + height; j++)
	{
		for (int i = x; i < x + width; i++)
		{
			if (m.Get(i, j).type == TileType::Nothing)
			{
				auto outputTile = output[j - y][i - x];
				if (outputTile == WFCTileType::Wall || outputTile == WFCTileType::Asteroid)
				{
					m.Set(i, j, Tile(TileType::WFCWall));
				}
				else if (outputTile == WFCTileType::Empty || outputTile == WFCTileType::Floor || outputTile == WFCTileType::NonRoomFloor)
				{
					m.Set(i, j, Tile(TileType::WFCFloor));
				}

				placed++;
			}
		}
	}

	return placed;
}

int WFCDungeonGenInterface::PlaceConstantVisualisation(int x, int y, int width, int height)
{
	const std::string& path = pattern_paths[DRandom::instance->GetInt(0, pattern_paths.size() - 1)];

	//Could alternatively use the Map to fill up WFC output with pre-collapsed values
	WaveFunctionCollapse wfc(path, width, height);

	int drawStep = 0;

	auto apply_to_map =
		[&](const boost::multi_array<WFCTileType::WFCTile, 2>& wfc)
	{
		int placed = 0;
		for (int j = y; j < y + height; j++)
		{
			for (int i = x; i < x + width; i++)
			{
				if (m.Get(i, j).type == TileType::Nothing || m.Get(i, j).type == TileType::WFCWall || m.Get(i, j).type == TileType::WFCFloor)
				{
					auto outputTile = wfc[j - y][i - x];
					if (outputTile == WFCTileType::Wall || outputTile == WFCTileType::Asteroid)
					{
						m.Set(i, j, Tile(TileType::WFCWall));
					}
					else if (outputTile == WFCTileType::Empty || outputTile == WFCTileType::Floor || outputTile == WFCTileType::NonRoomFloor)
					{
						m.Set(i, j, Tile(TileType::WFCFloor));
					}

					placed++;
				}
			}
		}
		if (drawStep++ % 20 == 0)
		{
			m.Display(false);
		}
		return placed;
	};

	auto output = wfc.RunVisualiseContinuous(apply_to_map);


	return apply_to_map(output);
}

}