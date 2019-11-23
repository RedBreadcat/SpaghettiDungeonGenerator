#pragma once
#include "Coord.h"
#include "MapArray.h"
#include "Tile.h"

class WallOffNothingAreas
{
public:
	static void Run();

private:
	static void FloodFill(Coord start, MapArray<Tile>& map_copy);
};