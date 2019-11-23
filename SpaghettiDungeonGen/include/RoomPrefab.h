#pragma once
#include <vector>
#include <tuple>
#include <array>
#include "PossibleDoor.h"
#include "TileWallData.h"
#include "RoomType.h"

#ifndef GAME_COMPILE
#include <opencv2/opencv.hpp>
#endif

class Map;

//Determines how many times we rotate the room while checking if it fits. We still rotate rooms even if they're NoRotations. 
enum class RotationType
{
	Asymmetrical = 0,
	OneAxisSymmetry = 1,
	TwoAxisSymmetry = 2
};

class RoomPrefab
{
public:
	RoomPrefab();
	RoomPrefab(RoomPrefab&& other) = default;

	static RoomPrefab CreateFromJSON(const std::string& path);

	bool PlaceWhereItFits(int x, int y, int prefab_index, bool strict);
	bool DirectPlace(int x, int y, int prefab_index, RoomType desired_type, bool strict);
	void Place(int x, int y, int current_rotation_element, int prefab_index);
	bool IsValid(int x, int y, int rotation_elemt);
	static void ExportImageToJSON(const std::string& image_path, const std::string& json_path);
	bool CanBePlacedOnTile(int x, int y, const std::vector<Coord>& tiles_to_test, bool strict);
	bool DoorsCanBePlacedOnTile(int x, int y, const std::vector<PossibleDoor>& doors, bool strict);
	void Reset();

	//Note: All the points here (and in RoomTileWallData) are offsets. Must add to the rooom's x,y to get map coordinates
	std::array<TileWallData, 4> room_tile_wall_data;	//0: 0 degrees, 1: 90 degrees, 2: 180 degrees, 3: 270 degrees
	std::array<std::vector<PossibleDoor>, 4> possible_doors;	//The place booleans must be reset every time the prefab is newly accessed
	std::vector<Coord> open_connections;
	int bounds_width = 0;
	int bounds_height = 0;
	bool placed = false;
	RotationType rotation_type;

#ifndef GAME_COMPILE
private:
	static bool IsDoorTile2(int i, int j, cv::Mat& img);
#endif
};

