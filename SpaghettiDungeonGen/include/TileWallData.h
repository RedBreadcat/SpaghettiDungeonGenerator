#pragma once
#include "Coord.h"
#include <vector>
class TileWallData
{
public:
	TileWallData();
	TileWallData(std::vector<Coord>& tiles, std::vector<Coord>& walls);
	std::vector<Coord> tiles;	//Doesn't overlap with the walls vector below
	std::vector<Coord> walls;	//Any possible doors won't be in the walls array. Whether placed or not. I think. Keep track of this for procedural and prefab
};

