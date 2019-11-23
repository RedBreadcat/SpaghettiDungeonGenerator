#include "Map.h"
#include "DRandom.h"
#include "Direction.h"
#include "Tile.h"
#include "RoomPlacer.h"
#include <random>
#include <iostream>
#ifndef GAME_COMPILE
#include <opencv2\highgui\highgui.hpp>
#endif

Map::Map() : MapArray()
{
}

void Map::Reset(int width, int height, RoomPlacer* room_placer)
{
#ifndef GAME_COMPILE
	cv::namedWindow("Display", cv::WINDOW_NORMAL);
	cv::resizeWindow("Display", width * 5, height * 5);
#endif

	this->room_placer = room_placer;
	if (this->width > 0)	//If already set, then we need to reset the various objects
	{
		for (int j = 0; j < height; ++j)
		{
			for (int i = 0; i < width; ++i)
			{
				Set(i, j, Tile());
			}
		}
		rooms = ConnectionNodeFilteredCollection<PlacedRoom>();
		corridors = ConnectionNodeFilteredCollection<Corridor>();
		nodes = std::list<std::unique_ptr<ConnectionNode>>();
		node_wrappers.clear();
		special_rooms.clear();
	}

	SetSize(width, height);
	nodes_added = 0;

#ifndef GAME_COMPILE
	displayImg = cv::Mat(height, width, CV_8UC3);	//Rows, cols
#endif
}

void Map::Display(bool done)
{
#ifndef GAME_COMPILE
	for (int j = 0; j < height; ++j)
	{
		cv::Vec3b* hybridRow = displayImg.ptr<cv::Vec3b>(j);

		for (int i = 0; i < width; ++i)
		{
			TileType tile_type = Get(i, j).type;
			int bH = 0;
			int gH = 0;
			int rH = 0;
			if (tile_type == TileType::RoomWall || tile_type == TileType::WFCWall)
			{
				bH = gH = rH = 255;
			}
			else if (tile_type == TileType::FreeWall)
			{
				bH = gH = rH = 0;
			}
			else if (tile_type == TileType::CorridorWall)
			{
				gH = rH = 200;
				bH = 255;
			}
			else if (tile_type == TileType::Door)
			{
				bH = gH = 60;
				rH = 200;
			}
			else if (tile_type == TileType::OpenConnection)
			{
				rH = 150;
				bH = gH = 127;
			}
			else if (tile_type == TileType::PossibleConnection)
			{
				bH = gH = 120;
				rH = 200;
			}
			else if (tile_type == TileType::RoomFloor)
			{
				RoomType type = static_cast<PlacedRoom&>(*Get(i, j).node).room_type;
				switch (type)
				{
					case RoomType::Start:
					{
						bH = rH = 120;
						gH = 200;
						break;
					}
					case RoomType::Boss:
					{
						bH = gH = 120;
						rH = 200;
						break;
					}
					case RoomType::Treasure:
					{
						rH = 255;
						gH = 180;
						bH = 60;
						break;
					}
					case RoomType::Shop:
					{
						rH = gH = 120;
						bH = 200;
						break;
					}
					case RoomType::Normal:
					{
						bH = gH = rH = 127;
						break;
					}
					case RoomType::Procedural:
					{
						rH = gH = 127;
						bH = 150;
						break;
					}
					default:
					{
						std::cout << "Unknown room type lookup in hybrid drawing: " << (int)type << std::endl;
					}
				}
			}
			else if (tile_type == TileType::CorridorFloor)
			{
				bH = gH = rH = 80;
			}
			else if (tile_type == TileType::Nothing || tile_type == TileType::CorridorDilatedToMaximum || tile_type == TileType::CorridorDilatedToMaximumPreviouslyWFC || tile_type == TileType::WFCFloor)
			{
				bH = gH = rH = 0;
			}
			else
			{
				std::cout << "Undefined display tile: " << (int)tile_type << std::endl;
			}

			hybridRow[i][0] = bH;
			hybridRow[i][1] = gH;
			hybridRow[i][2] = rH;
		}
	}

	//If writing to disk, can use:
	//ffmpeg -framerate 25 -i %000d.png -c:v libx264 -profile:v high -crf 20 -pix_fmt yuv420p -s 923x923 -sws_flags neighbor output.mp4
	imshow("Display", displayImg);

	if (done)
	{
		cv::waitKey(0);
	}
	else
	{
		cv::waitKey(1);
	}
#endif
}

void Map::ConnectRooms()	//Connect rooms that just happen to be near other rooms, or corridors. Special rooms won't be connected to each other since that leads to failed map gen
{
	for (auto& r : rooms)
	{
		r.ConnectToNearbyRoomsOrCorridors();
	}
}

void Map::EraseRoom(PlacedRoom& node, bool unplace)
{
	if (unplace)
	{
		node.Unplace();
	}

	rooms.size--;

	switch (node.room_type)
	{
		case RoomType::Normal:
			room_placer->room_prefabs[node.prefab_index].placed = false;
			break;
		case RoomType::Boss:
			room_placer->boss_room_prefabs[node.prefab_index].placed = false;
			break;
		case RoomType::Shop:
			room_placer->shop_room_prefabs[node.prefab_index].placed = false;
			break;
		case RoomType::Treasure:
			room_placer->treasure_room_prefabs[node.prefab_index].placed = false;
			break;
	}

	auto found = std::find_if(nodes.begin(), nodes.end(),
		[&node](const std::unique_ptr<ConnectionNode>& n) { return n.get() == &node; }
	);

	if (found != nodes.end())
	{
		nodes.erase(found);
	}
	else
	{
		std::cout << "Room not found in list of nodes" << std::endl;
		assert(false);
	}

	RemoveConnections(node);

	//Remove from special rooms map if applicable
	for (auto iter = special_rooms.begin(); iter != special_rooms.end(); iter++)
	{
		if (iter->second.get() == node)
		{
			special_rooms.erase(iter);
			break;
		}
	}
}

void Map::EraseCorridor(Corridor& node, bool unplace)
{
	if (unplace)
	{
		node.Unplace();
	}

	//cout << "Erased corridor: " << &node << "\n";
	corridors.size--;

	auto found = std::find_if(nodes.begin(), nodes.end(),
		[&node](const std::unique_ptr<ConnectionNode>& n) { return n.get() == &node; }
	);

	if (found != nodes.end())
	{
		nodes.erase(found);
	}
	else
	{
		std::cout << "Corridor not found in list of nodes" << std::endl;
		assert(false);
	}

	RemoveConnections(node);
}

void Map::RemoveConnections(ConnectionNode& node)
{
	RemoveWrapper(node);

	for (auto& r : rooms)
	{
		for (auto& d : r.possible_doors)
		{
			if (d.connection_opt == &node)
			{
				d.connection_opt = nullptr;
			}
		}
	}

	for (auto& c : corridors)
	{
		//Assuming there's no duplicates
		auto iter_connection_to_erase = std::find_if(c.connections.begin(), c.connections.end(), [&](std::reference_wrapper<ConnectionNode> ref_node)
		{
			return ref_node.get() == node;
		});

		if (iter_connection_to_erase != c.connections.end())
		{
			c.connections.erase(iter_connection_to_erase);
		}
	}
}

bool Map::SpecialRoomExists(RoomType type)
{
	return find_if(special_rooms.begin(), special_rooms.end(),
		[&](auto& pair)
	{
		return pair.first == type;
	}) != special_rooms.end();
}

PlacedRoom& Map::AddRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, TileWallData* tile_wall_data, int rotation, int prefab_index)
{
	nodes.emplace_front(std::make_unique<PlacedRoom>(x, y, doors_in, tile_wall_data, rotation, prefab_index));
	AddWrapper(*nodes.front().get());
	rooms.size++;
	//cout << "Added prefab room: " << (PlacedRoom*)nodes.front().get() << "\n";
	return *(PlacedRoom*)nodes.front().get();
}

PlacedRoom& Map::AddRoom(int x, int y, const std::vector<PossibleDoor>& doors_in, std::unique_ptr<TileWallData> tile_wall_data)
{
	nodes.emplace_front(std::make_unique<PlacedRoom>(x, y, doors_in, std::move(tile_wall_data)));
	AddWrapper(*nodes.front().get());
	rooms.size++;
	//cout << "Added proc room: " << (PlacedRoom*)nodes.front().get() << "\n";
	return *(PlacedRoom*)nodes.front().get();
}

Corridor& Map::AddCorridor(std::unique_ptr<ConnectionNode> node)
{
	nodes.emplace_front(move(node));
	AddWrapper(*nodes.front().get());
	corridors.size++;
	//cout << "Added corridor: " << (Corridor*)nodes.front().get() << "\n";
	return *(Corridor*)nodes.front().get();
}

void Map::AddWrapper(ConnectionNode& node)
{
	node_wrappers.push_back({nodes_added, node});
	nodes_added++;
}

void Map::RemoveWrapper(ConnectionNode& node)
{
	auto found = std::find_if(node_wrappers.begin(), node_wrappers.end(), [&](ConnectionNodeWrapper& wrapper)
	{
		return wrapper.node() == node;
	});

	node_wrappers.erase(found);
}

ConnectionNodeWrapper Map::NodeToWrapper(ConnectionNode& node)
{
	auto found = std::find_if(node_wrappers.begin(), node_wrappers.end(), [&](ConnectionNodeWrapper& wrapper)
	{
		return wrapper.node() == node;
	});

	if (found != node_wrappers.end())
	{
		return { found->id, node };
	}
	else
	{
		std::cout << "NodeToWrapper failed: " << &node << "\n";
		assert(false);
		throw;
	}
}

ConnectionNodeWrapper Map::IntToWrapper(int id)
{
	auto found = std::find_if(node_wrappers.begin(), node_wrappers.end(), [id](ConnectionNodeWrapper& wrapper)
	{
		return wrapper.id == id;
	});

	if (found != node_wrappers.end())
	{
		return *found;
	}
	else
	{
		std::cout << "IntToWrapper failed: " << id << "\n";
		assert(false);
		throw;
	}
}

ConnectionNode& Map::IntToWrapperNode(int id)
{
	auto found = std::find_if(node_wrappers.begin(), node_wrappers.end(), [id](ConnectionNodeWrapper& wrapper)
	{
		return wrapper.id == id;
	});

	if (found != node_wrappers.end())
	{
		return found->node();
	}
	else
	{
		std::cout << "IntToWrapperNode failed: " << id << "\n";
		assert(false);
		throw;
	}
}

int Map::NodeToWrapperInt(const ConnectionNode& node)
{
	auto found = std::find_if(node_wrappers.begin(), node_wrappers.end(), [&](ConnectionNodeWrapper& wrapper)
	{
		return wrapper.node() == node;
	});

	if (found != node_wrappers.end())
	{
		return found->id;
	}
	else
	{
		std::cout << "NodeToWrapperInt failed: " << &node << "\n";
		assert(false);
		throw;
	}
}
