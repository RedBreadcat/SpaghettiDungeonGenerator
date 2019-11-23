#include "EntropyGrid.h"
#include "DRandom.h"
#include <array>
#include <limits>

namespace WFC
{

EntropyGrid::EntropyGrid(int width, int height) :
	width(width),
	height(height)
{
	entropies_with_noise.resize(boost::extents[height][width]);
	noise.resize(boost::extents[height][width]);
}

void EntropyGrid::Create(const boost::multi_array<OutputTile, 2>& output, const std::vector<Pattern>& patterns)
{
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			Coord pos = { i, j };
			noise[j][i] = DRandom::instance->GetZeroOne();
			entropies_with_noise[j][i] = noise[j][i];
			for (size_t possible_pattern = 0; possible_pattern < output[j][i].possible_patterns.size(); possible_pattern++)
			{
				const auto& directions = output[j][i].possible_patterns[possible_pattern];
				entropies_with_noise[j][i] += patterns[possible_pattern].frequency * (directions[0] + directions[1] + directions[2] + directions[3]);
			}
			min_entropy_tracker.push({ pos, entropies_with_noise[j][i] });
		}
	}
}

void EntropyGrid::Collapse(Coord coord)
{
	entropies_with_noise[coord.y][coord.x] = 0;
}

EntropyGrid::LowestEntropyResult EntropyGrid::GetLowestEntropyPixel()
{
	while (!min_entropy_tracker.empty())
	{
		LowestEntropyResult result = min_entropy_tracker.top();
		min_entropy_tracker.pop();

		if (entropies_with_noise[result.position.y][result.position.x] != 0)
		{
			//Floating point stuff could mess this up
			return { result.position, result.value - noise[result.position.y][result.position.x] };
		}
	}

	return { {}, std::numeric_limits<float>::max() };
}

void EntropyGrid::ReduceEntropyNoUpdate(Coord pos, int amount)
{
	entropies_with_noise[pos.y][pos.x] -= amount;
}

void EntropyGrid::CheckIfCoordShouldBeNewMinimum(Coord pos)
{
	min_entropy_tracker.push({ pos, entropies_with_noise[pos.y][pos.x] });
}

bool operator<(const EntropyGrid::LowestEntropyResult& lhs, const EntropyGrid::LowestEntropyResult& rhs)
{
	//Opposite way around so we get a reversed priority queue
	return lhs.value > rhs.value;
}

}