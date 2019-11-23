#include "TileWallData.h"

TileWallData::TileWallData()
{
}

TileWallData::TileWallData(std::vector<Coord>& tiles, std::vector<Coord>& walls)
{
	this->tiles = std::move(tiles);
	this->walls = std::move(walls);
}
