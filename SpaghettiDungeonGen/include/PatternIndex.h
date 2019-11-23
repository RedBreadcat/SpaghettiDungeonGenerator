#pragma once
#include <array>
#include <vector>

#include <boost/multi_array.hpp>

#include "Pattern.h"
#include "Coord.h"
#include "Direction.h"

namespace WFC
{
class PatternIndex
{
public:
	void CreatePatternIndex(int n, const std::vector<Pattern>& patterns);
	bool CheckPatternConnectsToBaseline(int pattern_baseline, Direction direction, int pattern_other);

private:
	void TryAddPatternToIndex(int n, Coord offset, int pattern_baseline, int pattern_sliding, const std::vector<Pattern>& patterns);
	bool CheckPatternAtPositionCanOverlap(int n, Coord offset, const Pattern& pattern_baseline, const Pattern& pattern_sliding);

	//Note that the order of sliding then baseline is better for cache, due to the order WaveFunctionCollapse::PropagateToNeighbour iterates
	boost::multi_array<std::array<bool, 4>, 2> index;	//[sliding][baseline][0..3 directions] -> bool of whether pattern can be a neighbour
};
}