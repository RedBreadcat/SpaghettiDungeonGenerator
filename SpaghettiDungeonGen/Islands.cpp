#include "Islands.h"
#include "PlacedRoom.h"
#include "Map.h"
#include "DRandom.h"
#include <variant>

using namespace std;

extern Map m;

Islands::Islands(const std::map<vertex_descriptor_cg, int>& islands_map_in, int num_islands) :
	islands_map(islands_map_in)
{
	islands.resize(num_islands);

	for (auto& component : islands_map)
	{
		islands[component.second].push_back(m.IntToWrapperNode(component.first));
	}
}

bool Islands::ConnectIslands(int id1, int id2)
{
	//We're not connected 2 doors explicitly
	//Verify that each islands has potential doors available, then do a BFS to find the nearest door with a connection_node equal to a room in the island
	//The room_to_ignore stuff I have presently is wrong. Instead, should connect to a room that is within a vector of rooms on the target island.

	bool island1_has_connectable_available = false;	//Available door, corridor or open connectable
	for (auto node1 : islands[id1])
	{
		if (node1.get().IsRoom())
		{
			const PlacedRoom& room = static_cast<PlacedRoom&>(node1.get());
			if (room.HasConnectionAvailable())
			{
				island1_has_connectable_available = true;
				break;
			}
		}
		else //It's a corridor
		{
			island1_has_connectable_available = true;
			break;
		}
	}

	if (island1_has_connectable_available)
	{
		bool island2_has_connectable_available = false;
		for (auto node2 : islands[id2])
		{
			if (node2.get().IsRoom())
			{
				const PlacedRoom& room2 = static_cast<PlacedRoom&>(node2.get());
				if (room2.HasConnectionAvailable())
				{
					island2_has_connectable_available = true;
					break;
				}
			}
			else //It's a corridor
			{
				island2_has_connectable_available = true;
				break;
			}
		}

		if (island2_has_connectable_available)
		{
			//Select either a random corridor, door or open connection on island 1, and find a path to a connectable on island 2
			struct StartingAtCorridor {};

			Coord start;
			ConnectionNode* node_to_connect_to1 = nullptr;
			std::variant<PossibleDoor*, StartingAtCorridor> startConnectionType;

			for (auto node1 : islands[id1])
			{
				if (node1.get().IsRoom())
				{
					auto& room1 = static_cast<PlacedRoom&>(node1.get());

					//todo: randomly pick door instead?
					for (auto& d : room1.possible_doors)
					{
						if (!d.Connected())
						{
							start = room1.p + d.tile_outside_relative;
							node_to_connect_to1 = &room1;
							startConnectionType = &d;
							break;
						}
					}
				}
				else //It's a corridor
				{
					auto& corridor1 = static_cast<Corridor&>(node1.get());
					start = corridor1.tile_wall_data->tiles[DRandom::instance->GetInt(0, (int)corridor1.tile_wall_data->tiles.size() - 1)];
					node_to_connect_to1 = &node1.get();
					startConnectionType = StartingAtCorridor();
				}

				if (node_to_connect_to1 != nullptr)	//Valid node found
				{
					break;
				}
			}

			if (node_to_connect_to1 == nullptr)
			{
				assert(false && "node_to_connect_to_ptr1 should be defined");
			}

			optional<PathToNearestConnectable> path_opt = m.empty_space_pathfinder.GetPathToNearestConnectable(start, islands[id2]);
			optional<std::reference_wrapper<Corridor>> corridor_opt;
			if (path_opt)
			{
				switch (path_opt->connectedTileType)
				{
				case TileType::PossibleConnection:
				{
					PlacedRoom& room2 = static_cast<PlacedRoom&>(path_opt->node);
					PossibleDoor& door2 = room2.possible_doors[path_opt->index_of_connectable];
					corridor_opt = Corridor::TryToPlace(*node_to_connect_to1, room2, path_opt->path, 1);

					if (corridor_opt)
					{
						door2.SetConnectionWithCorridor(room2, *corridor_opt);
					}
					break;
				}
				case TileType::CorridorWall:
				{
					Corridor& corridor2 = static_cast<Corridor&>(path_opt->node);
					corridor_opt = Corridor::TryToPlace(*node_to_connect_to1, corridor2, path_opt->path, 1);	//Connect corridor to corridor
					break;
				}
				default:
					std::cout << "Invalid return from GetPathToNearestConnectable() " << (int)path_opt->connectedTileType << std::endl;
				}

				if (corridor_opt)
				{
					if (holds_alternative<PossibleDoor*>(startConnectionType))
					{
						get<PossibleDoor*>(startConnectionType)->SetConnectionWithCorridor(*node_to_connect_to1, *corridor_opt);
					}
					return true;
				}
			}
		}
	}

	return false;
}

void Islands::RemoveCorridorOnlyIslands()
{
	for (auto& island : islands)
	{
		bool erase_island = true;
		for (const ConnectionNode& node : island)
		{
			if (node.IsRoom())
			{
				erase_island = false;
				continue;
			}
		}

		if (erase_island)
		{
			for (ConnectionNode& node : island)
			{
				m.EraseCorridor(static_cast<Corridor&>(node), true);
			}
		}
	}
}