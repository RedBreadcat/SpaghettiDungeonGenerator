#pragma once
#include <stdint.h>

enum class TileType : uint8_t
{
	Nothing,
	RoomWall,
	RoomFloor,
	PossibleConnection,
	Door,
	OpenConnection,
	CorridorWall,
	CorridorFloor,
	CorridorDilatedToMaximum,
	CorridorDilatedToMaximumPreviouslyWFC,
	WFCFloor,
	WFCWall,
	FreeWall
};