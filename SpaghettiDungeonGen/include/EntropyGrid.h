#pragma once
#include <boost/multi_array.hpp>

#include <boost/heap/priority_queue.hpp>

#include "Coord.h"
#include "OutputTile.h"
#include "Pattern.h"

namespace WFC
{
class EntropyGrid
{
public:
	struct LowestEntropyResult
	{
		Coord position;
		float value;
	};

	EntropyGrid(int width, int height);
	void Create(const boost::multi_array<OutputTile, 2>& output, const std::vector<Pattern>& patterns);
	void Collapse(Coord coord);
	LowestEntropyResult GetLowestEntropyPixel();
	void ReduceEntropyNoUpdate(Coord pos, int amount);
	void CheckIfCoordShouldBeNewMinimum(Coord pos);

private:
	boost::heap::priority_queue<LowestEntropyResult> min_entropy_tracker;
	boost::multi_array<float, 2> entropies_with_noise;
	int width;
	int height;

	std::vector<Coord> remaining_entropies_ordered;
	boost::multi_array<float, 2> noise;
};

bool operator<(const EntropyGrid::LowestEntropyResult& lhs, const EntropyGrid::LowestEntropyResult& rhs);

}


