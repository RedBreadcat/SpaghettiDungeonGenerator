#pragma once
#include "Tile.h"
#include "MapArray.h"
#include "TileWallData.h"
#include "PossibleDoor.h"
#include <boost/multi_array.hpp>
#include <list>

enum class ProceduralRoomTileType
{
	Unprocessed,
	Nothing,
	Wall,
	PossibleConnection,
	Reserved
};

class RoomGenerator : public MapArray<ProceduralRoomTileType>
{
public:
	RoomGenerator(int x, int y, int width, int height);

	bool Generate();
	void AddSquare();

	static bool CheckWallShouldBePlaced(const MapArray<ProceduralRoomTileType>& map_array, int x, int y);
	static bool CanDilateWallIntoRoom(const MapArray<ProceduralRoomTileType>& map_array, int x, int y);

private:
	int CountTiles();
	void PlaceDoors();
	void PlaceSingleDoors();
	bool DoorCanBePlacedAtCaller(int i, int j);
	bool TileOutsideOfRoom(int i, int j);
	int room_x;
	int room_y;
	std::vector<PossibleDoor> possible_doors;
	TileWallData tile_wall_data;
};

