#ifdef GAME_COMPILE
#include "GameInterface.h"
#include "RoomPlacer.h"
#include "Map.h"

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

extern Map m;

//https://stackoverflow.com/questions/44464552/how-to-properly-marshal-strings-from-unity-to-c-c
extern "C"
{
	char* Generate(const char* load_disk_path, int width, int height, unsigned int seed, int min_rooms, int min_boss_distance, int min_treasure_distance, int min_shop_distance, int min_non_start_distance)
	{
		Requirements requirements;
		requirements.min_rooms = min_rooms;
		requirements.min_boss_distance = min_boss_distance;
		requirements.min_treasure_distance = min_treasure_distance;
		requirements.min_shop_distance = min_shop_distance;
		requirements.min_non_start_distance = min_non_start_distance;

		RoomPlacer rp(load_disk_path);

		rp.PlaceRooms(width, height, seed, requirements);

		std::string json = ExportMapToJSON();

		char* json_to_unity = new char[json.size() + 1];
		for (size_t i = 0; i < json.size(); i++)
		{
			json_to_unity[i] = json[i];
		}
		json_to_unity[json.size()] = '\0';

		return json_to_unity;
	}

	void Delete(char* str)
	{
		delete[] str;
	}
}

std::string ExportMapToJSON()
{
	rapidjson::Document doc;
	doc.SetObject();

	auto& a = doc.GetAllocator();

	rapidjson::Value v_rooms(rapidjson::Type::kArrayType);
	for (auto& room : m.rooms)
	{
		rapidjson::Value j_room(rapidjson::Type::kObjectType);
		j_room.AddMember("x", room.p.x, a);
		j_room.AddMember("y", room.p.y, a);
		j_room.AddMember("rotation", room.rotation, a);
		j_room.AddMember("prefab_index", room.prefab_index, a);
		j_room.AddMember("room_type", static_cast<int>(room.room_type), a);
		j_room.AddMember("room_connectivity", static_cast<int>(room.room_connectivity), a);

		TileWallData* data;
		if (std::holds_alternative<TileWallData*>(room.tile_wall_data))
		{
			data = std::get<TileWallData*>(room.tile_wall_data);
		}
		else
		{
			data = std::get<std::unique_ptr<TileWallData>>(room.tile_wall_data).get();
		}

		rapidjson::Value v_tiles(rapidjson::Type::kArrayType);
		for (auto& t : data->tiles)
		{
			rapidjson::Value j_tile(rapidjson::Type::kObjectType);
			j_tile.AddMember("x", t.x, a);
			j_tile.AddMember("y", t.y, a);
			v_tiles.PushBack(j_tile, a);
		}

		rapidjson::Value v_walls(rapidjson::Type::kArrayType);
		for (auto& w : data->walls)
		{
			rapidjson::Value j_wall(rapidjson::Type::kObjectType);
			j_wall.AddMember("x", w.x, a);
			j_wall.AddMember("y", w.y, a);
			v_walls.PushBack(j_wall, a);
		}

		rapidjson::Value v_doors(rapidjson::Type::kArrayType);
		rapidjson::Value v_open_connections(rapidjson::Type::kArrayType);
		for (auto& d : room.possible_doors)
		{
			if (d.open_connectable)
			{
				if (d.Connected())
				{
					rapidjson::Value j_connection(rapidjson::Type::kObjectType);
					j_connection.AddMember("x", d.tile1_relative.x, a);
					j_connection.AddMember("y", d.tile1_relative.y, a);
					v_open_connections.PushBack(j_connection, a);
				}
				else
				{
					rapidjson::Value j_wall(rapidjson::Type::kObjectType);
					j_wall.AddMember("x", d.tile1_relative.x, a);
					j_wall.AddMember("y", d.tile1_relative.y, a);
					v_walls.PushBack(j_wall, a);
				}
			}
			else
			{
				if (d.Connected())
				{
					rapidjson::Value j_door(rapidjson::Type::kObjectType);
					j_door.AddMember("x1", d.tile1_relative.x, a);
					j_door.AddMember("y1", d.tile1_relative.y, a);
					j_door.AddMember("x2", d.tile2_relative.x, a);
					j_door.AddMember("y2", d.tile2_relative.y, a);
					j_door.AddMember("f", (int)d.facing, a);
					v_doors.PushBack(j_door, a);
				}
				else
				{
					rapidjson::Value j_wall1(rapidjson::Type::kObjectType);
					j_wall1.AddMember("x", d.tile1_relative.x, a);
					j_wall1.AddMember("y", d.tile1_relative.y, a);
					v_walls.PushBack(j_wall1, a);
					//Connection may be useful

					if (d.facing != DoorFacing::SingleTile)
					{
						rapidjson::Value j_wall2(rapidjson::Type::kObjectType);
						j_wall2.AddMember("x", d.tile2_relative.x, a);
						j_wall2.AddMember("y", d.tile2_relative.y, a);
						v_walls.PushBack(j_wall2, a);
					}
				}
			}
		}
		j_room.AddMember("tiles", v_tiles, a);
		j_room.AddMember("walls", v_walls, a);
		j_room.AddMember("doors", v_doors, a);
		j_room.AddMember("open_connections", v_open_connections, a);

		v_rooms.PushBack(j_room, a);
	}
	doc.AddMember("Rooms", v_rooms, a);

	rapidjson::Value v_corridors(rapidjson::Type::kArrayType);
	for (auto& corridor : m.corridors)
	{
		rapidjson::Value j_corridor(rapidjson::Type::kObjectType);

		rapidjson::Value v_tiles(rapidjson::Type::kArrayType);
		for (auto& t : corridor.tile_wall_data->tiles)
		{
			rapidjson::Value j_tile(rapidjson::Type::kObjectType);
			j_tile.AddMember("x", t.x, a);
			j_tile.AddMember("y", t.y, a);
			v_tiles.PushBack(j_tile, a);
		}
		j_corridor.AddMember("tiles", v_tiles, a);

		rapidjson::Value v_walls(rapidjson::Type::kArrayType);
		for (auto& w : corridor.tile_wall_data->walls)
		{
			rapidjson::Value j_wall(rapidjson::Type::kObjectType);
			j_wall.AddMember("x", w.x, a);
			j_wall.AddMember("y", w.y, a);
			v_walls.PushBack(j_wall, a);
		}
		j_corridor.AddMember("walls", v_walls, a);

		v_corridors.PushBack(j_corridor, a);
	}
	doc.AddMember("Corridors", v_corridors, a);

	doc.AddMember("Width", m.width, a);
	doc.AddMember("Height", m.height, a);

	std::stringstream output;
	rapidjson::OStreamWrapper osw(output);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	doc.Accept(writer);

	return output.str();
}
#endif