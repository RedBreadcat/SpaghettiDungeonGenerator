#include "RoomPrefab.h"
#include "Map.h"
#include "DRandom.h"
#include "TileType.h"
#include "Tile.h"
#include "ConnectivityGrapher.h"
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <fstream>
#include <sstream>
#include <tuple>

extern Map m;

using namespace std;

RoomPrefab::RoomPrefab()
{

}

RoomPrefab RoomPrefab::CreateFromJSON(const std::string& path)
{
	RoomPrefab prefab_room;
	auto& possible_doors = prefab_room.possible_doors;

	rapidjson::Document doc;
	ifstream file(path);
	stringstream buffer;
	buffer << file.rdbuf();
	doc.Parse(buffer.str().c_str());

	switch (doc["rotation_type"].GetInt())
	{
		case 0:
			prefab_room.rotation_type = RotationType::Asymmetrical;
			break;
		case 1:
			prefab_room.rotation_type = RotationType::OneAxisSymmetry;
			break;
		case 2:
			prefab_room.rotation_type = RotationType::TwoAxisSymmetry;
			break;
		default:
			cout << "Rotation type not defined for " << path << "\n";
			assert(false);
	}

	prefab_room.room_tile_wall_data[0].tiles.reserve(doc["x"].Capacity());
	for (size_t i = 0; i < doc["x"].Capacity(); i++)
	{
		int x = doc["x"].GetArray()[(int)i].GetInt();
		int y = doc["y"].GetArray()[(int)i].GetInt();

		if (x > prefab_room.bounds_width)
		{
			prefab_room.bounds_width = x;
		}
		if (y > prefab_room.bounds_height)
		{
			prefab_room.bounds_height = y;
		}

		prefab_room.room_tile_wall_data[0].tiles.push_back(Coord{ x, y });
	}

	prefab_room.room_tile_wall_data[0].walls.reserve(doc["wallx"].Capacity());
	for (size_t i = 0; i < doc["wallx"].Capacity(); i++)
	{
		int x = doc["wallx"].GetArray()[(int)i].GetInt();
		int y = doc["wally"].GetArray()[(int)i].GetInt();

		if (x > prefab_room.bounds_width)
		{
			prefab_room.bounds_width = x;
		}
		if (y > prefab_room.bounds_height)
		{
			prefab_room.bounds_height = y;
		}

		prefab_room.room_tile_wall_data[0].walls.push_back(Coord{ x, y });
	}
	prefab_room.bounds_width++;	//Width and height are 1 more than the actual tile value, as done above
	prefab_room.bounds_height++;

	//90 degrees
	prefab_room.room_tile_wall_data[1].tiles.reserve(prefab_room.room_tile_wall_data[0].tiles.size());
	prefab_room.room_tile_wall_data[1].walls.reserve(prefab_room.room_tile_wall_data[0].walls.size());
	for (const auto& t : prefab_room.room_tile_wall_data[0].tiles)
	{
		prefab_room.room_tile_wall_data[1].tiles.push_back({ prefab_room.bounds_height - t.y - 1, t.x });
	}
	for (const auto& t : prefab_room.room_tile_wall_data[0].walls)
	{
		prefab_room.room_tile_wall_data[1].walls.push_back({ prefab_room.bounds_height - t.y - 1, t.x });
	}

	//180 degrees
	prefab_room.room_tile_wall_data[2].tiles.reserve(prefab_room.room_tile_wall_data[0].tiles.size());
	prefab_room.room_tile_wall_data[2].walls.reserve(prefab_room.room_tile_wall_data[0].walls.size());
	for (const auto& t : prefab_room.room_tile_wall_data[0].tiles)
	{
		int x90 = prefab_room.bounds_height - t.y - 1;
		int y90 = t.x;
		prefab_room.room_tile_wall_data[2].tiles.push_back({ prefab_room.bounds_width - y90 - 1, x90 });	//Using width instead of height since we've rotated by 90 degrees
	}
	for (const auto& t : prefab_room.room_tile_wall_data[0].walls)
	{
		int x90 = prefab_room.bounds_height - t.y - 1;
		int y90 = t.x;
		prefab_room.room_tile_wall_data[2].walls.push_back({ prefab_room.bounds_width - y90 - 1, x90 });
	}

	//270 degrees
	prefab_room.room_tile_wall_data[3].tiles.reserve(prefab_room.room_tile_wall_data[0].tiles.size());
	prefab_room.room_tile_wall_data[3].walls.reserve(prefab_room.room_tile_wall_data[0].walls.size());
	for (const auto& t : prefab_room.room_tile_wall_data[0].tiles)
	{
		prefab_room.room_tile_wall_data[3].tiles.push_back({ t.y, prefab_room.bounds_width - t.x - 1 });
	}
	for (const auto& t : prefab_room.room_tile_wall_data[0].walls)
	{
		prefab_room.room_tile_wall_data[3].walls.push_back({ t.y, prefab_room.bounds_width - t.x - 1 });
	}

	possible_doors[0].reserve(doc["doorx1"].Capacity());
	for (size_t i = 0; i < doc["doorx1"].Capacity(); i++)
	{
		int x1 = doc["doorx1"].GetArray()[(int)i].GetInt();
		int y1 = doc["doory1"].GetArray()[(int)i].GetInt();
		int x2 = doc["doorx2"].GetArray()[(int)i].GetInt();
		int y2 = doc["doory2"].GetArray()[(int)i].GetInt();
		possible_doors[0].push_back(PossibleDoor(x1, y1, x2, y2, prefab_room.room_tile_wall_data[0]));
	}

	for (size_t i = 0; i < doc["openconnectionsx"].Capacity(); i++)
	{
		int x = doc["openconnectionsx"].GetArray()[(int)i].GetInt();
		int y = doc["openconnectionsy"].GetArray()[(int)i].GetInt();
		possible_doors[0].push_back(PossibleDoor(x, y, -1, -1, prefab_room.room_tile_wall_data[0], true));
	}

	for (auto& d : possible_doors[0])
	{
		possible_doors[1].push_back(d.Get90RotatedCopy(prefab_room.bounds_height));
		possible_doors[2].push_back(possible_doors[1].back().Get90RotatedCopy(prefab_room.bounds_width));	//Height becomes width after the first rotation
		possible_doors[3].push_back(possible_doors[2].back().Get90RotatedCopy(prefab_room.bounds_height));
	}

	return prefab_room;
}

bool RoomPrefab::PlaceWhereItFits(int x, int y, int prefab_index, bool strict)
{
	Reset();
	int radius = 100;
	int d = DRandom::instance->GetInt(0, 3); //Direction; 0=RIGHT, 1=DOWN, 2=LEFT, 3=UP
	int s = 1; //Chain size

	for (int k = 1; k <= (radius - 1); k++)
	{
		for (int j = 0; j < (k < (radius - 1) ? 2 : 3); j++)
		{
			for (int i = 0; i < s; i++)
			{
				int times_to_rotate;
				switch (rotation_type)
				{
					case RotationType::Asymmetrical:
						times_to_rotate = 3;					//Will try all combinations
						break;
					case RotationType::OneAxisSymmetry:
						times_to_rotate = 1;					//Will rotate a single time
						break;
					case RotationType::TwoAxisSymmetry:
						times_to_rotate = 0;					//Won't rotate any times, since it won't affect failure
						break;
					default:
						assert(false && "Unknown RotationType");
						times_to_rotate = 3;
						break;
				}

				int current_rotation_element = DRandom::instance->GetInt(0, 3);
				while (true)
				{
					auto& data = room_tile_wall_data[current_rotation_element];
					auto& doors = possible_doors[current_rotation_element];

					if (IsValid(x, y, current_rotation_element) && CanBePlacedOnTile(x, y, data.tiles, strict) && CanBePlacedOnTile(x, y, data.walls, strict) && DoorsCanBePlacedOnTile(x, y, doors, strict))
					{
						Place(x, y, current_rotation_element, prefab_index);
						return true;
					}

					//Rotation code
					if (times_to_rotate-- == 0)
					{
						break;
					}
					else
					{
						current_rotation_element++;
						if (current_rotation_element == 4)
						{
							current_rotation_element = 0;
						}
					}
				}

				switch (d)
				{
					case 0: y++; break;
					case 1: x++; break;
					case 2: y--; break;
					case 3: x--; break;
				}
			}
			d = (d + 1) % 4;
		}
		s = s + 1;
	}

	return false;
}

bool RoomPrefab::DirectPlace(int x, int y, int prefab_index, RoomType desired_type, bool strict)
{
	PlacedRoom* first_room;

	if (m.Get(x, y).IsRoom())
	{
		first_room = static_cast<PlacedRoom*>(m.Get(x, y).node);
	}
	else
	{
		auto begin = m.rooms.begin();
		if (begin != m.rooms.end())
		{
			first_room = &*begin;
		}
		else
		{
			return false;	//No rooms exist
		}
	}

	auto existing_room_iter = find(m.rooms.begin(), m.rooms.end(), *first_room);

	bool looped = false;
	while (true)
	{
		if (existing_room_iter == m.rooms.end())
		{
			existing_room_iter = m.rooms.begin();
			looped = true;
		}

		if (looped && *existing_room_iter == *first_room)
		{
			return false;
		}

		PlacedRoom& existing_room = *existing_room_iter;

		//Only connect rooms if at least one of them is normal/procedural
		//Otherwise we'll have special rooms connected directly to each other
		if (existing_room.room_type == RoomType::Normal || desired_type == RoomType::Normal || existing_room.room_type == RoomType::Procedural || desired_type == RoomType::Procedural)
		{
			for (auto& existing_d : existing_room.possible_doors)
			{
				if (!existing_d.Connected())
				{
					int times_to_rotate;
					switch (rotation_type)
					{
						case RotationType::Asymmetrical:
							times_to_rotate = 3;					//Will try all combinations
							break;
						case RotationType::OneAxisSymmetry:
							times_to_rotate = 1;					//Will rotate a single time
							break;
						case RotationType::TwoAxisSymmetry:
							times_to_rotate = 0;					//Won't rotate any times, since it won't affect failure
							break;
					}

					int current_rotation_element = DRandom::instance->GetInt(0, 3);
					while (true)
					{
						auto& doors = possible_doors[current_rotation_element];
						for (auto& attempt_d : doors)
						{
							//Only attempt to place if the doors are facing in valid directions
							if (existing_d.facing == DoorFacing::Left && attempt_d.facing == DoorFacing::Right
								|| existing_d.facing == DoorFacing::Right && attempt_d.facing == DoorFacing::Left
								|| existing_d.facing == DoorFacing::Top && attempt_d.facing == DoorFacing::Bottom
								|| existing_d.facing == DoorFacing::Bottom && attempt_d.facing == DoorFacing::Top
								|| existing_d.facing == DoorFacing::SingleTile
								|| attempt_d.facing == DoorFacing::SingleTile)
							{
								Coord attempt = existing_room.p + existing_d.tile_outside_relative - attempt_d.GetOutsideAdjacentTile();

								auto& data = room_tile_wall_data[current_rotation_element];
								if (IsValid(attempt.x, attempt.y, current_rotation_element)
									&& CanBePlacedOnTile(attempt.x, attempt.y, data.tiles, strict)
									&& CanBePlacedOnTile(attempt.x, attempt.y, data.walls, strict)
									&& DoorsCanBePlacedOnTile(attempt.x, attempt.y, doors, strict))
								{
									Place(attempt.x, attempt.y, current_rotation_element, prefab_index);
									return true;
								}
							}
						}

						//Rotation code
						if (times_to_rotate-- == 0)
						{
							break;
						}
						else
						{
							current_rotation_element++;
							if (current_rotation_element == 4)
							{
								current_rotation_element = 0;
							}
						}
					};
				}
			}
		}

		++existing_room_iter;
	}

	return false;
}

void RoomPrefab::Reset()
{
	placed = false;
	for (auto& d_v : possible_doors)
	{
		for (auto& d : d_v)
		{
			d.connection_opt = nullptr;
		}
	}
}

bool RoomPrefab::CanBePlacedOnTile(int x, int y, const vector<Coord>& tiles_to_test, bool strict)	//Assumes tile is already valid
{
	bool found_a_tile_that_cannot_be_placed = any_of(tiles_to_test.begin(), tiles_to_test.end(), [&](const Coord& t)	//Using a parallel execution policy slows things down significantly.
	{
		if (strict)
		{
			return !m.Get(x + t.x, y + t.y).RoomCanBePlacedOnStrict();
		}
		else
		{
			return !m.Get(x + t.x, y + t.y).RoomCanBePlacedOnRelaxed();
		}
	});

	return !found_a_tile_that_cannot_be_placed;
}

//Check whether any doors can't be placed using any_of. This short-circuits.
bool RoomPrefab::DoorsCanBePlacedOnTile(int x, int y, const vector<PossibleDoor>& doors, bool strict)	//Assumes tile is already valid
{
	bool found_a_door_that_cannot_be_placed = any_of(doors.begin(), doors.end(), [&](const PossibleDoor& d)	//Using a parallel execution policy slows things down significantly.
	{
		bool cannot_be_placed;
		if (strict)
		{
			cannot_be_placed = !m.Get(x + d.tile1_relative.x, y + d.tile1_relative.y).RoomCanBePlacedOnStrict();
		}
		else
		{
			cannot_be_placed = !m.Get(x + d.tile1_relative.x, y + d.tile1_relative.y).RoomCanBePlacedOnRelaxed();
		}

		if (cannot_be_placed)
		{
			return true;
		}
		else //So far, we can place the door
		{
			if (d.facing == DoorFacing::SingleTile)
			{
				return false;	//If the door is single and we determined that t1 is no problem, then cannot_be_placed==false.
			}
			else
			{
				if (strict)
				{
					return !m.Get(x + d.tile2_relative.x, y + d.tile2_relative.y).RoomCanBePlacedOnStrict();
				}
				else
				{
					return !m.Get(x + d.tile2_relative.x, y + d.tile2_relative.y).RoomCanBePlacedOnRelaxed();
				}
			}
		}
	});

	return !found_a_door_that_cannot_be_placed;
}

void RoomPrefab::Place(int x, int y, int current_rotation_element, int prefab_index)
{
	m.AddRoom(x, y, possible_doors[current_rotation_element], &room_tile_wall_data[current_rotation_element], current_rotation_element * 90, prefab_index);
}

//The bounds are based on the crucial assumption that a wall tile will be at the edges of what is placed.
//Occurs in CreateFromJSON(), where I only check walls for the bounds width and height
bool RoomPrefab::IsValid(int x, int y, int current_rotation_element)
{
	switch (current_rotation_element)
	{
		case 0:
		case 2:
			return x >= 0 && y >= 0 && x + bounds_width <= m.width && y + bounds_height <= m.height;
		case 1:
		case 3:
			return x >= 0 && y >= 0 && x + bounds_height <= m.width && y + bounds_width <= m.height;
		default:
			cout << "Impossible current_rotation_element: " << current_rotation_element << "\n";
			assert(false);
			return false;
	}
}

void RoomPrefab::ExportImageToJSON(const std::string& image_path, const std::string& json_path)
{
#ifndef GAME_COMPILE
	rapidjson::Document doc;
	doc.SetObject();
	rapidjson::Value tiles_x(rapidjson::Type::kArrayType);
	rapidjson::Value tiles_y(rapidjson::Type::kArrayType);
	rapidjson::Value walls_x(rapidjson::Type::kArrayType);
	rapidjson::Value walls_y(rapidjson::Type::kArrayType);
	rapidjson::Value doors_x1(rapidjson::Type::kArrayType);
	rapidjson::Value doors_y1(rapidjson::Type::kArrayType);
	rapidjson::Value doors_x2(rapidjson::Type::kArrayType);
	rapidjson::Value doors_y2(rapidjson::Type::kArrayType);
	rapidjson::Value open_connections_x(rapidjson::Type::kArrayType);
	rapidjson::Value open_connections_y(rapidjson::Type::kArrayType);

	cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);

	int walls = 0;
	int iters = 0;

	for (int j = 0; j < img.rows; j++)
	{
		for (int i = 0; i < img.cols; i++)
		{
			iters++;
			cv::Vec3b px = img.at<cv::Vec3b>(j, i);
			uchar b = px[0];
			uchar g = px[1];
			uchar r = px[2];

			if (r == 255 && g == 255 && b == 255)	//Wall
			{
				walls_x.PushBack(i, doc.GetAllocator());
				walls_y.PushBack(j, doc.GetAllocator());
				walls++;
			}
			else if (r == 0 && g == 0 && b == 255)	//Floor
			{
				tiles_x.PushBack(i, doc.GetAllocator());
				tiles_y.PushBack(j, doc.GetAllocator());
			}
			else if (r == 0 && g == 255 && b == 0)	//Door
			{
				//i,j is x1,y1 but we need to find x2,y2 by searching nearby tiles. We also mark them, so we don't find them again later. Only need to search down and right, due to the order we traverse the image
				if (i + 1 < img.cols)
				{
					if (IsDoorTile2(i + 1, j, img))
					{
						doors_x1.PushBack(i, doc.GetAllocator());
						doors_y1.PushBack(j, doc.GetAllocator());
						doors_x2.PushBack(i + 1, doc.GetAllocator());
						doors_y2.PushBack(j, doc.GetAllocator());
						continue;
					}
				}
				if (j + 1 < img.rows)
				{
					if (IsDoorTile2(i, j + 1, img))
					{
						doors_x1.PushBack(i, doc.GetAllocator());
						doors_y1.PushBack(j, doc.GetAllocator());
						doors_x2.PushBack(i, doc.GetAllocator());
						doors_y2.PushBack(j + 1, doc.GetAllocator());
						continue;
					}
				}

				//It's a single-tile door
				doors_x1.PushBack(i, doc.GetAllocator());
				doors_y1.PushBack(j, doc.GetAllocator());
				doors_x2.PushBack(-1, doc.GetAllocator());
				doors_y2.PushBack(-1, doc.GetAllocator());
			}
			else if (r == 255 && g == 0 && b == 0)	//Open connection
			{
				open_connections_x.PushBack(i, doc.GetAllocator());
				open_connections_y.PushBack(j, doc.GetAllocator());
			}
			else if (r == 0 && g == 0 && b == 0)
			{
				//Nothing
			}
			else
			{
				cout << "Error at " << i << ", " << j << " we have the unknown tile (" << (int)r << ", " << (int)g << ", " << (int)b << ")" << endl;
				assert(false);	//Unknown tiles not allowed
			}
		}
	}

	auto images_equal = [](const cv::Mat& a, const cv::Mat& b) -> bool
	{
		for (int j = 0; j < a.rows; j++)
		{
			for (int i = 0; i < a.cols; i++)
			{
				if (a.at<cv::Vec3b>(j, i) != b.at<cv::Vec3b>(j, i))
				{
					return false;
				}
			}
		}
		return true;
	};

	cv::Mat flippedHorizontal, flippedVertical;
	cv::flip(img, flippedHorizontal, 0);
	cv::flip(img, flippedVertical, 1);
	int axesOfSymmetry = 0;
	if (images_equal(img, flippedHorizontal))
	{
		axesOfSymmetry++;
	}
	if (images_equal(img, flippedVertical))
	{
		axesOfSymmetry++;
	}

	auto& allocator = doc.GetAllocator();
	doc.AddMember("x", tiles_x, allocator);
	doc.AddMember("y", tiles_y, allocator);
	doc.AddMember("wallx", walls_x, allocator);
	doc.AddMember("wally", walls_y, allocator);
	doc.AddMember("doorx1", doors_x1, allocator);
	doc.AddMember("doory1", doors_y1, allocator);
	doc.AddMember("doorx2", doors_x2, allocator);
	doc.AddMember("doory2", doors_y2, allocator);
	doc.AddMember("openconnectionsx", open_connections_x, allocator);
	doc.AddMember("openconnectionsy", open_connections_y, allocator);
	doc.AddMember("rotation_type", rapidjson::Value(axesOfSymmetry).Move(), allocator);

	ofstream output(json_path);
	rapidjson::OStreamWrapper osw(output);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	doc.Accept(writer);
#endif
}

#ifndef GAME_COMPILE
bool RoomPrefab::IsDoorTile2(int i, int j, cv::Mat& img)
{
	cv::Vec3b& px = img.at<cv::Vec3b>(j, i);
	uchar& b = px[0];
	uchar& g = px[1];
	uchar& r = px[2];

	if (r == 0 && g == 255 && b == 0)
	{
		g = 0;	//Set to blank tile so we don't detect the door later
		return true;
	}

	return false;
}
#endif