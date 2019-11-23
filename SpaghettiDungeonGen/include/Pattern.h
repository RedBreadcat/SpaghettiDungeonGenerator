#pragma once
#include <boost/multi_array.hpp>

#include <vector>
#include "WFCTileType.h"

namespace WFC
{
class Pattern
{
public:
	Pattern(int n);
	boost::multi_array<WFCTileType, 2> tiles;	//NxN
	int frequency;

	size_t GetTileHash() const;
	void SetTileHash();
	Pattern GetRotated90() const;
	Pattern GetFlipped() const;

	size_t tile_hash;

	static std::vector<Pattern> LoadPatternsFromJSON(std::string_view path, int& n_out);
	static std::vector<Pattern> ExtractPatternsFromTexture(int n, const boost::multi_array<WFCTileType::WFCTile, 2>& texture);
	static void AddPattern(const Pattern& pattern, std::vector<Pattern>& patterns, std::unordered_map<size_t, int>& pattern_hash_to_index);
	static void ImageToJSON(std::string_view texture_path, std::string_view json_path, int n_force = -1);

};
}
