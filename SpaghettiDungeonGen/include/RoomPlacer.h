#pragma once
#include "RoomPrefab.h"
#include "RoomType.h"
#include "Requirements.h"
#include "WFCDungeonGenInterface.h"

enum class PlaceRoomResult
{
	Placed,
	NormalNotPlaced,
	SpecialNotPlaced
};

class RoomPlacer
{
public:
	RoomPlacer(std::string_view load_disk_path);
	void PlaceRooms(int w, int h, unsigned int seed, const Requirements& requirements);
	PlaceRoomResult DecideWhichRoomTypeToPlace(int approx_x, int approx_y, bool align_doors);
	bool PlaceRoomPrefab(int approx_x, int approx_y, bool align_doors, std::vector<RoomPrefab>& prefabs, RoomType type);
	bool PlaceProceduralRoom(int approx_x, int approx_y);
	void ConnectDeadEnds();
	bool ConnectSpecialRooms(bool &special_rooms_connected);
	bool FinaliseSpecialRooms(const Requirements& requirements);

	//Don't add or remove from this vector once they've been loaded. Pointers are stored to the elements inside. A resize may invalidate those pointers.
	std::vector<RoomPrefab> room_prefabs, boss_room_prefabs, treasure_room_prefabs, shop_room_prefabs;

private:
	std::vector<RoomPrefab> LoadRoomPrefabs(std::string_view rooms_path, std::string_view pattern);
	void HandleIsolatedRooms();
	void RemoveInaccessibleRooms();
	void EraseDeadEndCorridors();
	void SetRoomConnectivity();
	void Reset();

	WFC::WFCDungeonGenInterface wfc_interface;
};