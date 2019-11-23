#pragma once
#include "MapArray.h"

class BoringAreaRemover : public MapArray<int>
{
public:
	BoringAreaRemover();
	void Run();

private:
	void FloodFillToProduceDistanceMap(Coord start);
};

