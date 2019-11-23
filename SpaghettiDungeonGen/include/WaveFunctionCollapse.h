#pragma once
#include <boost/multi_array.hpp>
#include <unordered_map>
#include <vector>
#include <queue>
#include <optional>
#include "WFCTileType.h"
#include "PatternIndex.h"
#include "Coord.h"
#include "OutputTile.h"
#include "EntropyGrid.h"

namespace WFC
{
class WaveFunctionCollapse
{
public:
	WaveFunctionCollapse(std::string_view json_path, int width, int height);

	boost::multi_array<WFCTileType::WFCTile, 2> Run();
	boost::multi_array<WFCTileType::WFCTile, 2> RunVisualiseContinuous(std::function<void(boost::multi_array<WFCTileType::WFCTile, 2>)> callback);
	void RunVisualiseOnly();

private:
	struct Propagation
	{
		Coord pos;
		int pattern_lost;
	};

	std::optional<Coord> Observe();
	void Collapse(Coord pt);
	void Propagate();
	void PropagateToNeighbour(Coord baseline_pos, Coord offset, Direction direction, int pattern_lost);
	static constexpr Direction DirectionFromOffset(Coord offset);
	void VisualiseOutput();
	boost::multi_array<WFCTileType::WFCTile, 2> GetOutput();
	boost::multi_array<WFCTileType::WFCTile, 2> GetOutputDuring();

	boost::multi_array<OutputTile, 2> output;
	boost::multi_array<bool, 3> has_pattern;	//Dimensions are y, x, pattern index
	EntropyGrid entropy_grid;
	std::queue<Propagation> to_update;
	std::vector<Pattern> patterns;
	PatternIndex pattern_index;

	int width;
	int height;
	int n;
};
}