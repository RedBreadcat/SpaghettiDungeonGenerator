#pragma once
#include <boost/multi_array.hpp>
#include "TileType.h"
#include "Tile.h"
#include "Coord.h"

template <class T>
class MapArray
{
public:
	MapArray()
	{

	}

	MapArray& operator=(const MapArray& old_map_array)	//Copy assignment
	{
		this->width = old_map_array.width;
		this->height = old_map_array.height;
		this->data.resize(boost::extents[height][width]);	//Must resize before assigning
		this->data = old_map_array.data;
		return *this;
	}

	MapArray(const MapArray& old_map_array)			//Copy constructor
	{
		this->width = old_map_array.width;
		this->height = old_map_array.height;
		this->data.resize(boost::extents[height][width]);	//Must resize before assigning
		this->data = old_map_array.data;
	}

	void SetSize(int width, int height)
	{
		this->width = width;
		this->height = height;
		data.resize(boost::extents[height][width]);
	}

	bool IsValid(int x, int y) const
	{
		return x >= 0 && y >= 0 && x < width && y < height;
	}

	bool IsValid(Coord t) const
	{
		return IsValid(t.x, t.y);
	}

	T Get(int x, int y) const
	{
		return data[y][x];
	}

	T Get(Coord t) const
	{
		return data[t.y][t.x];
	}

	void Set(int x, int y, const T& tile)
	{
		data[y][x] = tile;
	}

	void Set(Coord t, const T& tile)
	{
		data[t.y][t.x] = tile;
	}

	boost::multi_array<T, 2>& GetDataReference()
	{
		return data;
	}
	
	int width;
	int height;
	boost::multi_array<T, 2> data;
};

