#include "WFCTileType.h"
#include <iostream>

namespace WFC
{
bool WFCTileType::operator==(const WFCTileType& other) const
{
	return tile == other.tile;
}

bool WFCTileType::operator!=(const WFCTileType& other) const
{
	return tile != other.tile;
}

std::tuple<uint8_t, uint8_t, uint8_t> WFCTileType::ToRGB() const
{
	switch (tile)
	{
	case Empty:
		return { 255, 255, 255 };
	case Wall:
		return { 255, 0, 0 };
	case Floor:
		return { 0, 255, 0 };
	case NonRoomFloor:
		return { 0, 0, 255 };
	case Asteroid:
		return { 255, 127, 0 };
	default:
		std::cout << "Unknown WFC tile" << std::endl;
		return { 255, 0, 255 };
	}
}

void WFCTileType::Set(uint8_t r, uint8_t g, uint8_t b)
{
	//Multiple definitions at different colours so patterns can be more flexible
	if (r == 255 && g == 255 && b == 255)
	{
		tile = Empty;
	}
	else if (r == 127 && g == 127 && b == 127)
	{
		tile = Empty;
	}
	else if (r == 255 && g == 0 && b == 0)
	{
		tile = Wall;
	}
	else if (r == 127 && g == 0 && b == 0)
	{
		tile = Wall;
	}
	else if (r == 0 && g == 255 && b == 0)
	{
		tile = Floor;
	}
	else if (r == 0 && g == 127 && b == 0)
	{
		tile = Floor;
	}
	else if (r == 0 && g == 0 && b == 255)
	{
		tile = NonRoomFloor;
	}
	else if (r == 0 && g == 0 && b == 127)
	{
		tile = NonRoomFloor;
	}
	else if (r == 255 && g == 127 && b == 0)
	{
		tile = Asteroid;
	}
	else
	{
		std::cout << "Unknown WFC tile colour " << (int)r << ' ' << (int)g << ' ' << (int)b << '\n';
	}
}

}