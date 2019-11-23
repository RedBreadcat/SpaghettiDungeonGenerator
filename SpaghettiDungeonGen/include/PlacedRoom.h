#pragma once
#include <vector>
#include <variant>
#include "PossibleDoor.h"
#include "ConnectivityGrapher.h"
#include "ConnectionNode.h"
#include "TileWallData.h"
#include "RoomType.h"

class PlacedRoom : public ConnectionNode
{
public:
	PlacedRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, TileWallData* tile_wall_data, int rotation_in, int prefab_index);
	PlacedRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, std::unique_ptr<TileWallData> tile_wall_data);
	PlacedRoom(PlacedRoom&& placed_room) = delete;	//Don't want to move, since I keep track of pointers. Don't want to accidentally use a pointer from before a move
	PlacedRoom& operator=(PlacedRoom&& placed_room) = delete;
	PlacedRoom& operator=(const PlacedRoom&) = delete;	//Never want to copy a PlacedRoom
	PlacedRoom(PlacedRoom& placed_room) = delete;

	bool IsRoom() const override;
	bool IsCorridor() const override;
	void Unplace();
	void ConnectToNearbyRoomsOrCorridors();
	void TryPlaceDoor(PossibleDoor& d);
	//void TryConnectOpenConnections(OpenConnection& o);
	int UnconnectedDoors() const;
	bool HasConnectionAvailable() const;

	Coord p;
	int rotation;	//Used by C#
	int prefab_index;	
	RoomType room_type = RoomType::Normal;
	std::vector<PossibleDoor> possible_doors;

	//Only set at the end of room placement
	enum class RoomConnectivity
	{
		Regular = 0,
		DeadEnd,
		Isolated
	};
	RoomConnectivity room_connectivity = RoomConnectivity::Regular;


	//Used to access positions of tiles and walls.
	//Raw: Points to data that is fixed at startup, like unmodified room prefabs
	//unique_ptr: Points to modified prefab data or proc gen room data. Stored in this object.
	std::variant<TileWallData*, std::unique_ptr<TileWallData>> tile_wall_data;

private:
	void Place();
};

