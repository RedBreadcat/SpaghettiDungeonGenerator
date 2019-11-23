#ifndef GAME_COMPILE
#include <iostream>
#include <boost/random/random_device.hpp>
#include <boost/filesystem.hpp>
#include "FileManager.h"
#include "RoomPlacer.h"
#include "Pattern.h"
#include "WaveFunctionCollapse.h"
#include "DRandom.h"


int main(int argc, char* argv[])
{
	if (argc == 3)
	{
		boost::random::random_device device;
		uint32_t seed = device();

		boost::filesystem::path tex_path = argv[1];
		boost::filesystem::path json_path = boost::filesystem::change_extension(tex_path, "json");

		int n = std::stoi(argv[2]);
		WFC::Pattern::ImageToJSON(tex_path.string(), json_path.string(), n);

		DRandom rand(seed);
		WFC::WaveFunctionCollapse wfc(json_path.string(), 64, 64);
		wfc.RunVisualiseOnly();
	}
	else if (argc == 1)
	{
		FileManager::ConvertPatternsToJSON(true);
		FileManager::ConvertRoomsToJSON(true);

		while (true)
		{
			boost::random::random_device device;
			uint32_t seed = device();
			//uint32_t seed = 3082447256;
			std::cout << "Seed: " << seed << "\n";

			RoomPlacer rp("C:\\Users\\Example\\Documents\\git\\SpaghettiDungeonGen\\");
			Requirements requirements;
			requirements.min_rooms = 30;
			requirements.min_boss_distance = 4;
			requirements.min_treasure_distance = 3;
			requirements.min_shop_distance = 2;
			requirements.min_non_start_distance = 2;
			rp.PlaceRooms(120, 120, seed, requirements);
		}
	}
	else
	{
		std::cout << "Unknown number of arguments" << std::endl;
	}

	return 0;
}
#endif