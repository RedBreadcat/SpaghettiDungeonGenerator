#pragma once
#include <vector>
#include "Direction.h"

struct CorridorInfo
{
	int thickness;
	std::vector<std::tuple<int, int, Direction>> tiles;	//x, y, direction
};

