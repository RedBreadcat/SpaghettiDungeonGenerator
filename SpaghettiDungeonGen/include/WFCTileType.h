#pragma once
#include <cstdint>
#include <tuple>

namespace WFC
{
struct WFCTileType
{
	enum WFCTile
	{
		Empty,
		Floor,
		Wall,
		NonRoomFloor,
		Asteroid
	};

	WFCTile tile;

	bool operator==(const WFCTileType& other) const;
	bool operator!=(const WFCTileType& other) const;

	std::tuple<uint8_t, uint8_t, uint8_t> ToRGB() const;
	void Set(uint8_t r, uint8_t g, uint8_t b);
};
}