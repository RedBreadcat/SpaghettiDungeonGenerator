#pragma once
#include "Coord.h"
#include "MapArray.h"
#include "ConnectivityGrapher.h"
#include "ConnectionNode.h"
#include "TileWallData.h"
#include <vector>
#include <optional>
#include <boost/multi_array.hpp>

class Tile;

class Corridor : public ConnectionNode
{
public:
	Corridor(ConnectionNode& start_node, ConnectionNode& end_node, std::vector<Coord>& tiles_in);
	static std::optional<std::reference_wrapper<Corridor>> TryToPlace(ConnectionNode& start_node, ConnectionNode& end_node, Coord start, Coord end, int dilation_iterations);
	static std::optional<std::reference_wrapper<Corridor>> TryToPlace(ConnectionNode& start_node, ConnectionNode& end_node, const std::vector<Coord>& path_tiles, int dilation_iterations);
	static void DilateTile(int x, int y, MapArray<Tile>& map_data, std::vector<Coord>& dilated_tiles, size_t& failed_dilations);
	static void DilateTiles(const std::vector<Coord>& tiles_to_dilate, MapArray<Tile>& map_data, std::vector<Coord>& dilated_tiles, size_t& failed_dilations);
	static void CheckTileWhenPlacing(int x, int y, MapArray<Tile>& map_data, std::vector<Coord>& dilated_tiles, size_t& failed_dilations);
	static void CheckTileForWallPlacement(Coord neighbour, std::vector<Coord>& dilated_tiles);
	void CheckTileForDilationToMaximum(Coord current, Coord neighbour, std::vector<Coord>& previously_dilated, std::vector<Coord>& dilated_tiles, std::vector<Coord>& dilated_into_walls);
	void DilateToMaximum();
	void PlaceWalls();

	bool IsRoom() const override;
	bool IsCorridor() const override;
	std::unique_ptr<TileWallData> tile_wall_data;

	void Unplace();
	void RePlaceMetadata();
	void AddConnection(ConnectionNode& node);

	std::vector<std::reference_wrapper<ConnectionNode>> connections;
};