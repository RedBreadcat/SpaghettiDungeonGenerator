#include "RoomPlacer.h"
#include "DRandom.h"
#include "Corridor.h"
#include "RoomGenerator.h"
#include "Islands.h"
#include "Map.h"
#include "WallOffNothingAreas.h"
#include "BoringAreaRemover.h"
#include <boost/filesystem.hpp>
#include <regex>

Map m;

RoomPlacer::RoomPlacer(std::string_view load_disk_path) :
	wfc_interface(load_disk_path)
{
	std::string rooms_path = std::string(load_disk_path) + "\\Rooms";
	room_prefabs = LoadRoomPrefabs(rooms_path, "room(\\d+)");
	boss_room_prefabs = LoadRoomPrefabs(rooms_path, "boss(\\d+)");
	treasure_room_prefabs = LoadRoomPrefabs(rooms_path, "treasure(\\d+)");
	shop_room_prefabs = LoadRoomPrefabs(rooms_path, "shop(\\d+)");
}

std::vector<RoomPrefab> RoomPlacer::LoadRoomPrefabs(std::string_view rooms_path, std::string_view pattern)
{
	using namespace boost::filesystem;

	using path_number = std::pair<std::string, int>;

	std::vector<path_number> room_paths_with_file_number;

	std::regex r(pattern.data());
	for (directory_iterator itr(rooms_path.data()); itr != directory_iterator(); itr++)
	{
		if (is_regular_file(itr->path()) && extension(itr->path()) == ".json")
		{
			std::string path = itr->path().string();
			std::smatch i_match;
			if (regex_search(path, i_match, r))
			{
				int number = stoi(i_match[1]);

				room_paths_with_file_number.push_back({ path, number });
			}
		}
	}

	sort(room_paths_with_file_number.begin(), room_paths_with_file_number.end(), 
		[](path_number& a, path_number& b)
	{
		return a.second < b.second;
	}
	);

	std::vector<RoomPrefab> prefabs;
	for (auto& p : room_paths_with_file_number)
	{
		prefabs.emplace_back(RoomPrefab::CreateFromJSON(p.first));
	}

	return prefabs;
}

void RoomPlacer::PlaceRooms(int w, int h, unsigned int seed, const Requirements& requirements)
{
	DRandom rng(seed);
	m.Reset(w, h, this);
	Reset();

	bool align_to_doors = true;
	int island_connection_failures = 0;
	int failures_in_a_row = 0;
	int total_failures = 0;
	int max_wfc_tiles = static_cast<int>(w * h * 0.15f);
	int current_wfc_tiles = 0;

	int prev_rand_x = DRandom::instance->GetInt(0, w);
	int prev_rand_y = DRandom::instance->GetInt(0, h);

	while (true)
	{
		bool updated = false;
		switch (DecideWhichRoomTypeToPlace(prev_rand_x, prev_rand_y, align_to_doors))
		{
			case PlaceRoomResult::Placed:
			{
				m.ConnectRooms();
				align_to_doors = true;
				failures_in_a_row = 0;

				prev_rand_x = m.rooms.recent().p.x + 4;
				prev_rand_y = m.rooms.recent().p.y + 4;
				updated = true;
				break;
			}
			case PlaceRoomResult::SpecialNotPlaced:
			{
				failures_in_a_row += 20;
				total_failures += 20;
			}
			case PlaceRoomResult::NormalNotPlaced:
			{
				failures_in_a_row++;
				total_failures++;
				prev_rand_x += DRandom::instance->GetInt(-20, 20);
				prev_rand_y += DRandom::instance->GetInt(-20, 20);
				break;
			}
		}

		if (prev_rand_x >= w || prev_rand_x < 0)
		{
			prev_rand_x = DRandom::instance->GetInt(0, w - 1);
			align_to_doors = false;
		}
		if (prev_rand_y >= h || prev_rand_y < 0)
		{
			prev_rand_y = DRandom::instance->GetInt(0, h - 1);
			align_to_doors = false;
		}

		if (m.rooms.size % 10 == 0)
		{
			RemoveInaccessibleRooms();
			ConnectDeadEnds();
			m.ConnectRooms();
			updated = true;
		}

		bool special_rooms_connected = false;
		if (m.rooms.size > 10)
		{
			if (ConnectSpecialRooms(special_rooms_connected))
			{
				updated = true;
			}
		}

		if (m.rooms.size > 20 && m.rooms.size % 5 == 0)
		{
			auto islands = m.connectivity_grapher.GetIslands();

			if (islands.islands.size() > 1)	//If there's more than one island
			{
				while (true)	//Try to connect 2 random islands. The random loop is so that we don't try to connect an island to itself
				{
					int island1 = DRandom::instance->GetInt(0, (int)islands.islands.size() - 1);
					int island2 = DRandom::instance->GetInt(0, (int)islands.islands.size() - 1);

					if (island1 != island2)
					{
						if (islands.ConnectIslands(island1, island2))
						{
							island_connection_failures = 0;
							updated = true;
						}
						else
						{
							island_connection_failures++;
						}
						break;
					}
				}
			}
		}

		if (island_connection_failures > 2)
		{
			HandleIsolatedRooms();
			island_connection_failures = 0;
			updated = true;
		}

		if (current_wfc_tiles < max_wfc_tiles && DRandom::instance->GetZeroOne() > 0.9f)
		{
			int wfc_width = DRandom::instance->GetInt(20, 80);
			int wfc_height = DRandom::instance->GetInt(20, 80);
			int wfc_x = DRandom::instance->GetInt(0, m.width - wfc_width);
			int wfc_y = DRandom::instance->GetInt(0, m.height - wfc_height);

			current_wfc_tiles += wfc_interface.Place(wfc_x, wfc_y, wfc_width, wfc_height);
			updated = true;
		}

		//TODO smartly erase corridors if there's too many of them

		if (failures_in_a_row > 100)
		{
			EraseDeadEndCorridors();
			if (m.rooms.size > requirements.min_rooms && special_rooms_connected)	//If we keep failing, all special rooms have been successfully placed, and they've all been connected then exit loop
			{
				m.ConnectRooms();
				if (FinaliseSpecialRooms(requirements))
				{
					//Under current implementation once the start room is placed, generation must be complete.
					//One possible issue is related to room-deletion and setting the prefab "placed" value, given that the room type is different. 
					break;
				}
				else
				{
					for (int deletions = 0; deletions < 2 && m.rooms.size > 0; deletions++)
					{
						m.EraseRoom(m.rooms.random(), true);
					}

					for (int deletions = 0; deletions < 2 && m.corridors.size > 0; deletions++)
					{
						m.EraseCorridor(m.corridors.random(), true);
					}
				}
			}
			else //If we fail a lot and the pass conditions aren't met, remove a few rooms and corridors
			{
				for (int deletions = 0; deletions < 5 && m.rooms.size > 0; deletions++)
				{
					m.EraseRoom(m.rooms.random(), true);
				}

				for (int deletions = 0; deletions < 2 && m.corridors.size > 0; deletions++)
				{
					m.EraseCorridor(m.corridors.random(), true);
				}

				failures_in_a_row = 0;
				updated = true;
			}
		}

		if (updated)
		{
			m.Display(false);
		}
	}
	PossibleDoor::PlaceAllDoorTiles();

	for (auto& c : m.corridors)
	{
		if (rng.GetZeroOne() > 0.5f)
		{
			c.DilateToMaximum();
		}
		else
		{
			c.PlaceWalls();
		}
	}

	WallOffNothingAreas::Run();

	BoringAreaRemover boringRemover;
	boringRemover.Run();

	SetRoomConnectivity();

	m.connectivity_grapher.Export();

	m.Display(true);
}

PlaceRoomResult RoomPlacer::DecideWhichRoomTypeToPlace(int approx_x, int approx_y, bool align_doors)
{
	bool place_special;
	if (m.rooms.size > 10)
	{
		if (m.special_rooms.size() == 0)
		{
			place_special = true;
		}
		else if (m.special_rooms.size() == 3)
		{
			place_special = false;
		}
		else
		{
			place_special = DRandom::instance->GetZeroOne() > m.special_rooms.size() / 3.0f;	//1 room: [0,1] > 0.33f, 2 rooms: [0, 1] > 0.66f, 3 rooms: [0, 1] > 0.99f
		}
	}
	else
	{
		place_special = false;
	}

	if (place_special)
	{
		bool special_align;
		if (m.rooms.size == 0)	//When no rooms are placed, the special room can be unaligned
		{
			special_align = false;
		}
		else
		{
			special_align = true;
		}

		if (!m.SpecialRoomExists(RoomType::Boss))	//Boss, treasure and shop rooms must be aligned
		{
			if (PlaceRoomPrefab(approx_x, approx_y, special_align, boss_room_prefabs, RoomType::Boss))
			{
				m.special_rooms.push_back(std::make_pair(RoomType::Boss, std::ref(m.rooms.recent())));
				return PlaceRoomResult::Placed;
			}
		}
		else if (!m.SpecialRoomExists(RoomType::Treasure))
		{
			if (PlaceRoomPrefab(approx_x, approx_y, special_align, treasure_room_prefabs, RoomType::Treasure))
			{
				m.special_rooms.push_back(std::make_pair(RoomType::Treasure, std::ref(m.rooms.recent())));
				return PlaceRoomResult::Placed;
			}
		}
		else if (!m.SpecialRoomExists(RoomType::Shop))
		{
			if (PlaceRoomPrefab(approx_x, approx_y, special_align, shop_room_prefabs, RoomType::Shop))
			{
				m.special_rooms.push_back(std::make_pair(RoomType::Shop, std::ref(m.rooms.recent())));
				return PlaceRoomResult::Placed;
			}
		}
		return PlaceRoomResult::SpecialNotPlaced;
	}
	else
	{
		if (DRandom::instance->GetZeroOne() > 0.25f)	//Place prefabs most of the time
		{
			if (PlaceRoomPrefab(approx_x, approx_y, align_doors, room_prefabs, RoomType::Normal))
			{
				return PlaceRoomResult::Placed;
			}
		}
		else
		{
			if (PlaceProceduralRoom(approx_x, approx_y))
			{
				return PlaceRoomResult::Placed;
			}
		}

		return PlaceRoomResult::NormalNotPlaced;
	}
}

bool RoomPlacer::PlaceRoomPrefab(int approx_x, int approx_y, bool align_doors, std::vector<RoomPrefab>& prefabs, RoomType type)
{
	int r;
	
	int row = 0;
	do
	{
		row++;
		r = DRandom::instance->GetInt(0, (int)prefabs.size() - 1);	//Go through prefabs until we find one that hasn't been placed yet
	} while (prefabs[r].placed);
	
	//If strict, room can overlap and overwrite WFC walls
	bool strict = DRandom::instance->GetZeroOne() > 0.005f;

	bool placed;
	if (align_doors)
	{
		placed = prefabs[r].DirectPlace(approx_x, approx_y, r, type, strict);
	}
	else
	{
		placed = prefabs[r].PlaceWhereItFits(approx_x, approx_y, r, strict);
	}

	if (placed)
	{
		prefabs[r].placed = true;
		m.rooms.recent().room_type = type;
		return true;
	}
	else
	{
		return false;
	}
}

bool RoomPlacer::PlaceProceduralRoom(int approx_x, int approx_y)
{
	int min_width = 15;
	int max_width = 20;
	int min_height = 15;
	int max_height = 20;
	int x2 = DRandom::instance->GetInt(approx_x + min_width, approx_x + max_width);
	int y2 = DRandom::instance->GetInt(approx_y + min_height, approx_y + max_height);

	if (x2 <= m.width && y2 <= m.height)
	{
		RoomGenerator g = RoomGenerator(approx_x, approx_y, x2 - approx_x, y2 - approx_y);
		return g.Generate();
	}
	else
	{
		return false;
	}
}

void RoomPlacer::ConnectDeadEnds()
{
	std::vector<std::reference_wrapper<ConnectionNode>> dead_ends = m.connectivity_grapher.GetNodesWithNeighbourCount(1);

	std::vector<std::pair<std::reference_wrapper<PlacedRoom>, std::reference_wrapper<PlacedRoom>>> connection_pairs;	//room1, room2
	if (dead_ends.size() > 0)	//The -1 below causes size=0 to underflow for unsigned
	{
		for (size_t i = 0; i < dead_ends.size() - 1; i++)
		{
			if (dead_ends[i].get().IsRoom())
			{
				auto& room1 = static_cast<PlacedRoom&>(dead_ends[i].get());
				auto room_graph_distance_map = m.connectivity_grapher.GetDistancesFromNode(room1, true);
				for (size_t j = i + 1; j < dead_ends.size(); j++)
				{
					if (dead_ends[j].get().IsRoom())
					{
						auto& room2 = static_cast<PlacedRoom&>(dead_ends[j].get());
						if (abs(room1.p.x - room2.p.x) < 30 && abs(room1.p.y - room2.p.y) < 30)
						{
							connection_pairs.push_back({ room1, room2 });
						}
					}
				}
			}
		}
	}

	for (auto& p : connection_pairs)
	{
		PlacedRoom& r1_room = p.first;
		PlacedRoom& r2_room = p.second;
		bool skip_pair = false;
		if (&r1_room != &r2_room)
		{
			for (auto& door1 : r1_room.possible_doors)
			{
				if (!door1.Connected())
				{
					if (m.IsValid(r1_room.p + door1.tile_outside_relative))
					{
						for (auto& door2 : r2_room.possible_doors)
						{
							if (!door2.Connected())
							{
								if (m.IsValid(r2_room.p + door2.tile_outside_relative))
								{
									auto corridor_opt = Corridor::TryToPlace(
										r1_room, r2_room,
										r1_room.p + door1.tile_outside_relative,
										r2_room.p + door2.tile_outside_relative,
										1);

									if (corridor_opt)
									{
										door1.SetConnectionWithCorridor(r1_room, *corridor_opt);
										door2.SetConnectionWithCorridor(r2_room, *corridor_opt);
										return;
									}
									else
									{
										skip_pair = true;
										break;
									}
								}
							}
						}
					}
				}

				if (skip_pair)
				{
					break;
				}
			}
		}
	}
}

bool RoomPlacer::ConnectSpecialRooms(bool &special_rooms_connected)
{
	bool updated = false;
	m.connectivity_grapher.GetIslands().RemoveCorridorOnlyIslands();

	//Connect special rooms
	if (m.special_rooms.size() > 1)
	{
		auto islands = m.connectivity_grapher.GetIslands();
		bool connection_made = false;

		for (auto iter1 = m.special_rooms.begin(); iter1 != prev(m.special_rooms.end(), 1) && !connection_made; iter1++) //i = 0; i < size - 1
		{
			for (auto iter2 = next(iter1, 1); iter2 != m.special_rooms.end(); iter2++)	//j = i + 1; j < size
			{
				int iter1_island = islands.islands_map[m.NodeToWrapperInt(iter1->second)];
				int iter2_island = islands.islands_map[m.NodeToWrapperInt(iter2->second)];

				if (iter1_island != iter2_island)
				{
					if (islands.ConnectIslands(iter1_island, iter2_island))
					{
						connection_made = true;
						updated = true;
						break;
					}
				}
			}
		}

		if (m.special_rooms.size() == 3 && !connection_made)
		{
			special_rooms_connected = true;
		}
	}
	return updated;
}

bool RoomPlacer::FinaliseSpecialRooms(const Requirements& requirements)
{
	int boss_room, treasure_room, shop_room;
	for (auto& special_room : m.special_rooms)
	{
		switch (special_room.first)
		{
			case RoomType::Boss:
				boss_room = m.NodeToWrapperInt(special_room.second);
				break;
			case RoomType::Treasure:
				treasure_room = m.NodeToWrapperInt(special_room.second);
				break;
			case RoomType::Shop:
				shop_room = m.NodeToWrapperInt(special_room.second);
				break;
		}
	}

	auto islands = m.connectivity_grapher.GetIslands();

	//Get the island that contains the boss room (and thus all special rooms)
	int island = islands.islands_map[boss_room];

	for (ConnectionNode& node : islands.islands[island])
	{
		if (node.IsRoom())
		{
			auto distance_map = m.connectivity_grapher.GetDistancesFromNode(node, false);

			if (distance_map[boss_room] >= requirements.min_boss_distance && distance_map[treasure_room] >= requirements.min_treasure_distance && distance_map[shop_room] >= requirements.min_shop_distance)	//Minimum distance these rooms must be away
			{
				//The special rooms must be away from each other too
				auto boss_distance_map = m.connectivity_grapher.GetDistancesFromNode(boss_room, false);
				if (boss_distance_map[shop_room] >= requirements.min_non_start_distance && boss_distance_map[treasure_room] >= requirements.min_non_start_distance)
				{
					if (m.connectivity_grapher.GetDistancesFromNode(treasure_room, false)[shop_room] >= 2)
					{
						PlacedRoom& room = static_cast<PlacedRoom&>(node);
						room.room_type = RoomType::Start;
						m.special_rooms.push_back({ RoomType::Start , room});
						return true;
					}
					else
					{
						//Unplace treasure room because it's too close to shop
						m.EraseRoom(static_cast<PlacedRoom&>(m.IntToWrapperNode(treasure_room)), true);
						return false;
					}
				}
				else
				{
					//Unplace boss room because it's too close to either the shop or the treasure room
					m.EraseRoom(static_cast<PlacedRoom&>(m.IntToWrapperNode(boss_room)), true);
					return false;
				}
			}
		}
	}

	m.EraseRoom(static_cast<PlacedRoom&>(m.IntToWrapperNode(boss_room)), true);
	m.EraseRoom(static_cast<PlacedRoom&>(m.IntToWrapperNode(treasure_room)), true);
	m.EraseRoom(static_cast<PlacedRoom&>(m.IntToWrapperNode(shop_room)), true);
	return false;
}

void RoomPlacer::HandleIsolatedRooms()
{
	auto isolated = m.connectivity_grapher.GetNodesWithNeighbourCount(0);

	for (auto node : isolated)
	{
		if (node.get().IsRoom())
		{
			m.EraseRoom(static_cast<PlacedRoom&>(node.get()), true);
		}
		else
		{
			std::cout << "Should not happen. All corridor-only islands should be removed: " << &node << "\n";
			assert(false);
		}
	}
}

void RoomPlacer::RemoveInaccessibleRooms()	//An inaccessible room has no doors. May add more conditions later.
{
	bool repeat;
	do
	{
		repeat = false;
		for (auto& r : m.rooms)
		{
			if (r.possible_doors.size() == 0)
			{
				m.EraseRoom(r, true);
				repeat = true;
				break;					//Once a room is erased, the for-loop becomes invalidated because the current iterator no longer exists, and therefore cannot be incremented. We then repeat so we don't miss anything

			}
		}
	} while (repeat);
}

void RoomPlacer::EraseDeadEndCorridors()
{
	std::vector<std::reference_wrapper<ConnectionNode>> dead_ends = m.connectivity_grapher.GetNodesWithNeighbourCount(1);

	for (auto& node : dead_ends)
	{
		if (node.get().IsCorridor())
		{
			m.EraseCorridor(static_cast<Corridor&>(node.get()), true);
		}
	}
}

void RoomPlacer::SetRoomConnectivity()
{
	std::vector<std::reference_wrapper<ConnectionNode>> dead_ends = m.connectivity_grapher.GetNodesWithNeighbourCount(1);

	for (auto& node : dead_ends)
	{
		if (node.get().IsRoom())
		{
			PlacedRoom& room = static_cast<PlacedRoom&>(node.get());
			room.room_connectivity = PlacedRoom::RoomConnectivity::DeadEnd;
		}
		else
		{
			std::cout << "At this stage of dungeon generation, corridors shouldn't be dead ends\n";
		}
	}

	std::vector<std::reference_wrapper<ConnectionNode>> isolated = m.connectivity_grapher.GetNodesWithNeighbourCount(0);

	for (auto& node : isolated)
	{
		if (node.get().IsRoom())
		{
			PlacedRoom& room = static_cast<PlacedRoom&>(node.get());
			room.room_connectivity = PlacedRoom::RoomConnectivity::Isolated;
		}
		else
		{
			std::cout << "At this stage of dungeon generation, corridors shouldn't be isolated\n";
		}
	}
}

void RoomPlacer::Reset()
{
	for (auto& p : room_prefabs)
	{
		p.Reset();
	}

	for (auto& p : boss_room_prefabs)
	{
		p.Reset();
	}

	for (auto& p : treasure_room_prefabs)
	{
		p.Reset();
	}

	for (auto& p : shop_room_prefabs)
	{
		p.Reset();
	}
}