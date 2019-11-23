#include "FileManager.h"
#include "RoomPrefab.h"
#include "Pattern.h"
#include <boost/filesystem.hpp>
#include <iostream>

using namespace boost::filesystem;

namespace FileManager
{

void ConvertRoomsToJSON(bool full_refresh)
{
	directory_iterator end_iter;
	for (directory_iterator itr("C:\\Users\\Example\\Documents\\git\\SpaghettiDungeonGen\\Rooms"); itr != end_iter; itr++)
	{
		auto json_path = change_extension(itr->path(), "json");
		if (is_regular_file(itr->path()) && extension(itr->path()) == ".png")
		{
			//Don't re-write JSON, unless full_refresh is true
			if (full_refresh || !boost::filesystem::exists(json_path))
			{
				RoomPrefab::ExportImageToJSON(itr->path().string(), json_path.string());
			}
		}
	}
}

void ConvertPatternsToJSON(bool full_refresh)
{
	directory_iterator end_iter;
	for (directory_iterator itr("C:\\Users\\Example\\Documents\\git\\SpaghettiDungeonGen\\Patterns"); itr != end_iter; itr++)
	{
		auto json_path = change_extension(itr->path(), "json");
		if (is_regular_file(itr->path()) && extension(itr->path()) == ".png")
		{
			//Don't re-write JSON, unless full_refresh is true
			if (full_refresh || !boost::filesystem::exists(json_path))
			{
				WFC::Pattern::ImageToJSON(itr->path().string(), json_path.string());
			}
		}
	}
}

}

