#include "PatternIndex.h"

namespace WFC
{
Direction DirectionFromOffset(Coord offset)
{
	if (offset.x == -1 && offset.y == 0)
	{
		return Direction::Left;
	}
	else if (offset.x == 1 && offset.y == 0)
	{
		return Direction::Right;
	}
	else if (offset.x == 0 && offset.y == -1)
	{
		return Direction::Up;
	}
	else if (offset.x == 0 && offset.y == 1)
	{
		return Direction::Down;
	}
	else
	{
		//TODO BAD
		return Direction::Left;
	}
}

void PatternIndex::CreatePatternIndex(int n, const std::vector<Pattern>& patterns)
{
	index.resize(boost::extents[patterns.size()][patterns.size()]);

	for (int pattern_a = 0; pattern_a < patterns.size(); pattern_a++)
	{
		for (int pattern_b = 0; pattern_b < patterns.size(); pattern_b++)
		{
			TryAddPatternToIndex(n, { -1, 0 }, pattern_a, pattern_b, patterns);
			TryAddPatternToIndex(n, { 1, 0 }, pattern_a, pattern_b, patterns);
			TryAddPatternToIndex(n, { 0, -1 }, pattern_a, pattern_b, patterns);
			TryAddPatternToIndex(n, { 0, 1 }, pattern_a, pattern_b, patterns);
		}
	}
}

bool PatternIndex::CheckPatternConnectsToBaseline(int pattern_baseline, Direction direction, int pattern_other)
{
	return index[pattern_other][pattern_baseline][static_cast<int>(direction)];
}

void PatternIndex::TryAddPatternToIndex(int n, Coord offset, int pattern_baseline, int pattern_sliding, const std::vector<Pattern>& patterns)
{
	if (CheckPatternAtPositionCanOverlap(n, offset, patterns[pattern_baseline], patterns[pattern_sliding]))
	{
		Direction direction = DirectionFromOffset(offset);
		index[pattern_sliding][pattern_baseline][static_cast<int>(direction)] = true;
	}
}

bool PatternIndex::CheckPatternAtPositionCanOverlap(int n, Coord offset, const Pattern& pattern_baseline, const Pattern& pattern_sliding)
{
	for (int baseline_y = 0; baseline_y < n; baseline_y++)
	{
		int comparison_y = baseline_y - offset.y;
		if (comparison_y >= 0 && comparison_y < n)
		{
			for (int baseline_x = 0; baseline_x < n; baseline_x++)
			{
				int comparison_x = baseline_x - offset.x;
				if (comparison_x >= 0 && comparison_x < n)
				{
					//(baseline_x, baseline_y) is overlapping
					//The equivalent tile for the sliding pattern is baseline-offset
					int sliding_x = baseline_x - offset.x;
					int sliding_y = baseline_y - offset.y;

					if (pattern_baseline.tiles[baseline_y][baseline_x] != pattern_sliding.tiles[sliding_y][sliding_x])
					{
						//Tiles don't match
						return false;
					}
				}
			}
		}
	}
	return true;
}
}
