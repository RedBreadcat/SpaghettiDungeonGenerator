#pragma once
#include <list>
#include "MapArray.h"
#include "PlacedRoom.h"
#include "Corridor.h"
#include "EmptySpacePathfinder.h"
#include "ConnectionNodeFilteredCollection.h"
#include "RoomPlacer.h"

#ifndef GAME_COMPILE
#include <opencv2\core\mat.hpp>
#endif

class Map : public MapArray<Tile>
{
public:
	Map();

	void Reset(int width, int height, RoomPlacer* room_placer);
	void Display(bool done);
	void ConnectRooms();
	void EraseRoom(PlacedRoom& node, bool unplace);
	void EraseCorridor(Corridor& node, bool unplace);
	void RemoveConnections(ConnectionNode& node);
	bool SpecialRoomExists(RoomType type);
	PlacedRoom& AddRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, TileWallData* tile_wall_data, int rotation, int prefab_index);
	PlacedRoom& AddRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, std::unique_ptr<TileWallData> tile_wall_data);
	Corridor& AddCorridor(std::unique_ptr<ConnectionNode> node);

	void AddWrapper(ConnectionNode& node);
	void RemoveWrapper(ConnectionNode& node);

	ConnectivityGrapher connectivity_grapher;
	EmptySpacePathfinder empty_space_pathfinder;
	ConnectionNodeFilteredCollection<PlacedRoom> rooms;
	ConnectionNodeFilteredCollection<Corridor> corridors;

	std::vector<std::pair<RoomType, std::reference_wrapper<PlacedRoom>>> special_rooms;	//Start, boss, treasure, shop.

	std::list<std::unique_ptr<ConnectionNode>> nodes;
	std::vector<ConnectionNodeWrapper> node_wrappers;
	ConnectionNodeWrapper NodeToWrapper(ConnectionNode& node);
	ConnectionNodeWrapper IntToWrapper(int id);
	ConnectionNode& IntToWrapperNode(int id);
	int NodeToWrapperInt(const ConnectionNode& node);

private:
	RoomPlacer* room_placer;
	Map& operator=(const Map&) = delete;	//Stop Map object from being copyable
	Map(const Map&) = delete;

#ifndef GAME_COMPILE
	cv::Mat displayImg;
#endif

public:
	int nodes_added = 0;
};