#pragma once
#include <string>

#ifdef GAME_COMPILE
extern "C"
{
	__declspec(dllexport) char* Generate(const char* load_disk_path, int width, int height, unsigned int seed, int min_rooms, int min_boss_distance, int min_treasure_distance, int min_shop_distance, int min_non_start_distance);
	__declspec(dllexport) void Delete(char* str);
}

std::string ExportMapToJSON();
#endif