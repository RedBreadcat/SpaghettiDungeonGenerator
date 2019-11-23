#pragma once
#include <string>
#include <vector>

namespace WFC
{

class WFCDungeonGenInterface
{
public:
	WFCDungeonGenInterface(std::string_view load_disk_path);
	int Place(int x, int y, int width, int height);
	int PlaceConstantVisualisation(int x, int y, int width, int height);

private:
	std::vector<std::string> pattern_paths;
};

}