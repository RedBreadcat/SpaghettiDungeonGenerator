#include "Tile.h"

Tile::Tile() :
	type(TileType::Nothing),
	node(nullptr)
{
}

Tile::Tile(TileType type) :
	type(type),
	node(nullptr)
{
}

Tile::Tile(TileType type, ConnectionNode* node) :
	type(type),
	node(node)
{
}

bool Tile::IsPathable() const
{
	return type == TileType::Nothing || type == TileType::WFCFloor || type == TileType::WFCWall || type == TileType::CorridorFloor;	//WFCWall is pathable, because we can cut right through it
}

bool Tile::RoomCanBePlacedOnStrict() const
{
	return (type == TileType::Nothing || type == TileType::WFCFloor);
}

bool Tile::RoomCanBePlacedOnRelaxed() const
{
	return (type == TileType::Nothing || type == TileType::WFCWall || type == TileType::WFCFloor);
}

bool Tile::IsRoom() const
{
	return type == TileType::RoomFloor || type == TileType::RoomWall;
}

ConnectionNode& Tile::GetNode() const
{
	return *node;
}