#pragma once
#include <optional>
#include <vector>
namespace WFC
{
struct OutputTile
{
	std::optional<int> collapsed_pattern;
	std::vector<std::array<int, 4>> possible_patterns;		//Pattern index, number of patterns in each direction that add to entropy
};
}