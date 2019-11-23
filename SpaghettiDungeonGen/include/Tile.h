#pragma once
#include "TileType.h"
#include "ConnectivityGrapher.h"
#include "ConnectionNode.h"

class Tile
{
public:
	Tile();
	Tile(TileType type);
	Tile(TileType type, ConnectionNode* node);

	bool IsPathable() const;
	bool RoomCanBePlacedOnStrict() const;
	bool RoomCanBePlacedOnRelaxed() const;
	bool IsRoom() const;
	ConnectionNode& GetNode() const;

	TileType type;
	ConnectionNode* node;
};