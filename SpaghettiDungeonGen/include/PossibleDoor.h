#pragma once
#include "ConnectivityGrapher.h"
#include "Coord.h"
#include "TileWallData.h"
#include <optional>

class ConnectionNode;
class Corridor;

enum class DoorFacing
{
	Top,
	Right,
	Bottom,
	Left,
	SingleTile
};

struct PossibleDoor
{
	Coord tile1_relative;	//Coordinates are relative to room
	Coord tile2_relative;	//Is -1, -1 when door is a single tile
	Coord tile_outside_relative;	//The tile just outside of the room
	DoorFacing facing;
	ConnectionNode* connection_opt;
	bool open_connectable;

	PossibleDoor();
	PossibleDoor(int x1, int y1, int x2, int y2, TileWallData& tiles_and_walls, bool open_connectable = false);
	bool SetOutsideTile(int x, int y, TileWallData& tiles_and_walls);
	void SetConnectionWithCorridor(ConnectionNode& node_this, Corridor& corridor);
	void SetConnectionWithAnotherDoor(PossibleDoor& door_other, ConnectionNode& node_this, ConnectionNode& node_other);
	bool Connected() const;
	PossibleDoor Get90RotatedCopy(int room_height) const;
	void Place(Coord room, ConnectionNode& node) const;
	void Unplace(Coord room) const;
	Coord GetOutsideAdjacentTile() const;

	static void PlaceAllDoorTiles();
};
