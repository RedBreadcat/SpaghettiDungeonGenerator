#pragma once
#include <map>
#include <vector>
#include "ConnectivityGrapher.h"
#include "ConnectionNode.h"

class Islands
{
public:
	Islands(const std::map<vertex_descriptor_cg, int>& islands_map_in, int num_islands);

	std::map<vertex_descriptor_cg, int> islands_map;
	std::vector<std::vector<std::reference_wrapper<ConnectionNode>>> islands;

	bool ConnectIslands(int id1, int id2);
	void RemoveCorridorOnlyIslands();
};