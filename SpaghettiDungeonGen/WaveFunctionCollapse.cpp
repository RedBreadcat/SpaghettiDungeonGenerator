#include "WaveFunctionCollapse.h"
#include "DRandom.h"
#include <limits>
#include <algorithm>
#ifndef GAME_COMPILE
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#endif

namespace WFC
{
WaveFunctionCollapse::WaveFunctionCollapse(std::string_view json_path, int width, int height) :
	width(width),
	height(height),
	entropy_grid(width, height)
{
/*#ifndef GAME_COMPILE
	cv::namedWindow("WFC", cv::WINDOW_NORMAL);
	cv::resizeWindow("WFC", width*2, height*2);
#endif*/
	patterns = Pattern::LoadPatternsFromJSON(json_path, n);
	pattern_index.CreatePatternIndex(n, patterns);

	output.resize(boost::extents[height][width]);
	has_pattern.resize(boost::extents[height][width][patterns.size()]);

	for (int i = 0; i < has_pattern.num_elements(); i++)
	{
		has_pattern.data()[i] = true;
	}

	//Stick the initial values into (0,0)
	output[0][0].possible_patterns.resize(patterns.size());
	for (int p1 = 0; p1 < patterns.size(); p1++)
	{
		for (int p2 = 0; p2 < patterns.size(); p2++)
		{
			if (pattern_index.CheckPatternConnectsToBaseline(p1, (Direction)0, p2))
			{
				output[0][0].possible_patterns[p1][0]++;
			}
			if (pattern_index.CheckPatternConnectsToBaseline(p1, (Direction)1, p2))
			{
				output[0][0].possible_patterns[p1][1]++;
			}
			if (pattern_index.CheckPatternConnectsToBaseline(p1, (Direction)2, p2))
			{
				output[0][0].possible_patterns[p1][2]++;
			}
			if (pattern_index.CheckPatternConnectsToBaseline(p1, (Direction)3, p2))
			{
				output[0][0].possible_patterns[p1][3]++;
			}
		}
	}

	//Copy all of (0,0)'s values to the rest of the grid. It's all the same at the start.
	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			output[j][i].possible_patterns = output[0][0].possible_patterns;
		}
	}

	entropy_grid.Create(output, patterns);
}

boost::multi_array<WFCTileType::WFCTile, 2> WaveFunctionCollapse::Run()
{
	while (true)
	{
		auto to_collapse_opt = Observe();

		if (to_collapse_opt)
		{
			Collapse(*to_collapse_opt);
			Propagate();
		}
		else
		{
			return GetOutput();
		}
	}
}

boost::multi_array<WFCTileType::WFCTile, 2> WaveFunctionCollapse::RunVisualiseContinuous(std::function<void(boost::multi_array<WFCTileType::WFCTile, 2>)> callback)
{
	while (true)
	{
		auto to_collapse_opt = Observe();

		if (to_collapse_opt)
		{
			Collapse(*to_collapse_opt);
			Propagate();
			callback(GetOutputDuring());
		}
		else
		{
			return GetOutput();
		}
	}
}

void WaveFunctionCollapse::RunVisualiseOnly()
{
	while (true)
	{
		auto to_collapse_opt = Observe();

		if (to_collapse_opt)
		{
			Collapse(*to_collapse_opt);
			Propagate();
			VisualiseOutput();
		}
		else
		{
			while (true)
			{
				VisualiseOutput();
			}
		}
	}
}

std::optional<Coord> WaveFunctionCollapse::Observe()
{
	EntropyGrid::LowestEntropyResult lowest_entropy_result = entropy_grid.GetLowestEntropyPixel();

	//TODO: this will probably need to be different
	//Search didn't find anything, so we're done
	if (lowest_entropy_result.value == std::numeric_limits<float>::max())
	{
		return std::nullopt;
	}

	//Out of the possible patterns in the tile, pick one at random (with weighting) and collapse
	int accumulated_entropy = 0;
	float pixel_to_choose = DRandom::instance->GetFloat(0, lowest_entropy_result.value);
	const auto& output_tile = output[lowest_entropy_result.position.y][lowest_entropy_result.position.x];
	for (int possible_pattern = 0; possible_pattern < output_tile.possible_patterns.size(); possible_pattern++)
	{
		const auto& directions = output_tile.possible_patterns[possible_pattern];

		accumulated_entropy += patterns[possible_pattern].frequency * (directions[0] + directions[1] + directions[2] + directions[3]);

		if (accumulated_entropy >= pixel_to_choose)
		{
			output[lowest_entropy_result.position.y][lowest_entropy_result.position.x].collapsed_pattern.emplace(possible_pattern);
			break;
		}
	}

	return lowest_entropy_result.position;
}

void WaveFunctionCollapse::Collapse(Coord pt)
{
	int collapsed_pattern = *output[pt.y][pt.x].collapsed_pattern;
	for (size_t possible_pattern = 0; possible_pattern < output[pt.y][pt.x].possible_patterns.size(); possible_pattern++)
	{
		if (possible_pattern != collapsed_pattern)
		{
			has_pattern[pt.y][pt.x][possible_pattern] = false;
			if (output[pt.y][pt.x].possible_patterns[possible_pattern][0] > 0 || output[pt.y][pt.x].possible_patterns[possible_pattern][1] > 0 ||
				output[pt.y][pt.x].possible_patterns[possible_pattern][2] > 0 || output[pt.y][pt.x].possible_patterns[possible_pattern][3] > 0)
			{
				to_update.push({ pt, (int)possible_pattern });
			}
		}
	}

	output[pt.y][pt.x].possible_patterns.clear();
	entropy_grid.Collapse(pt);
}

void WaveFunctionCollapse::Propagate()
{
	while (to_update.size() > 0)
	{
		auto [coord, pattern_lost] = to_update.front();
		to_update.pop();

		std::array<Coord, 4> offsets = { { Coord{-1, 0}, Coord{1, 0}, Coord{0, -1}, Coord{0, 1} } };
		for (Coord offset : offsets)
		{
			Direction direction = DirectionFromOffset(offset);
			PropagateToNeighbour(coord, offset, direction, pattern_lost);
		}
	}
}

void WaveFunctionCollapse::PropagateToNeighbour(Coord baseline_pos, Coord offset, Direction direction, int pattern_lost)
{
	Coord neighbour_pos = baseline_pos + offset;
	if (neighbour_pos.x < 0 || neighbour_pos.x >= width
		|| neighbour_pos.y < 0 || neighbour_pos.y >= height)
	{
		return;
	}

	Direction opposite;
	switch (direction)
	{
	case Direction::Down:
		opposite = Direction::Up;
		break;
	case Direction::Up:
		opposite = Direction::Down;
		break;
	case Direction::Left:
		opposite = Direction::Right;
		break;
	case Direction::Right:
		opposite = Direction::Left;
		break;
	default:
		std::cout << "Invalid direction case\n";
		opposite = Direction::Left;
	}

	bool push_to_entropy_grid_minimums = false;
	std::vector<std::array<int, 4>>& neighbour_possibilities = output[neighbour_pos.y][neighbour_pos.x].possible_patterns;
	for (int possible_pattern = 0; possible_pattern < neighbour_possibilities.size(); possible_pattern++)
	{
		if (has_pattern[neighbour_pos.y][neighbour_pos.x][possible_pattern])
		{
			//If we can connect to the pattern we just lost, then we better remove that pattern
			if (pattern_index.CheckPatternConnectsToBaseline(possible_pattern, opposite, pattern_lost))
			{
				int& element = neighbour_possibilities[possible_pattern][static_cast<int>(opposite)];
				element--;
				//Entropy has reduced by 1 * patterns[possible_pattern].frequency
				entropy_grid.ReduceEntropyNoUpdate(neighbour_pos, patterns[possible_pattern].frequency);
				push_to_entropy_grid_minimums = true;

				if (element == 0)
				{
					to_update.push({ neighbour_pos, possible_pattern });

					float entropy_reduction = patterns[possible_pattern].frequency *
						(neighbour_possibilities[possible_pattern][0] + neighbour_possibilities[possible_pattern][1] + neighbour_possibilities[possible_pattern][2] + neighbour_possibilities[possible_pattern][3]);
					entropy_grid.ReduceEntropyNoUpdate(neighbour_pos, entropy_reduction);

					//possible_pattern is no longer possible
					neighbour_possibilities[possible_pattern][0] = 0;
					neighbour_possibilities[possible_pattern][1] = 0;
					neighbour_possibilities[possible_pattern][2] = 0;
					neighbour_possibilities[possible_pattern][3] = 0;
					has_pattern[neighbour_pos.y][neighbour_pos.x][possible_pattern] = false;

					//Count the number of patterns remaining. If one, then collapse
					int collapsed_id = -1;
					int remaining_patterns = 0;
					for (int p = 0; p < patterns.size(); p++)
					{
						if (neighbour_possibilities[p][0] > 0 || neighbour_possibilities[p][1] > 0 || neighbour_possibilities[p][2] > 0 || neighbour_possibilities[p][3] > 0)
						{
							collapsed_id = p;
							remaining_patterns++;

							if (remaining_patterns > 1)
							{
								break;
							}
						}
					}

					if (remaining_patterns == 1)
					{
						push_to_entropy_grid_minimums = false;
						entropy_grid.Collapse(neighbour_pos);
						output[neighbour_pos.y][neighbour_pos.x].collapsed_pattern = collapsed_id;
						output[neighbour_pos.y][neighbour_pos.x].possible_patterns.clear();
						break;
					}
				}
			}
		}
	}

	if (push_to_entropy_grid_minimums)
	{
		entropy_grid.CheckIfCoordShouldBeNewMinimum(neighbour_pos);
	}
}

constexpr Direction WaveFunctionCollapse::DirectionFromOffset(Coord offset)
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
		std::cout << "Invalid offset input\n";
		return Direction::Left;
	}
}

void WaveFunctionCollapse::VisualiseOutput()
{
#ifndef GAME_COMPILE
	cv::Mat image(height, width, CV_8UC3);
	for (int j = 0; j < image.rows; j++)
	{
		cv::Vec3b* row = image.ptr<cv::Vec3b>(j);
		for (int i = 0; i < image.cols; i++)
		{
			if (output[j][i].collapsed_pattern)
			{
				int patternIndex = *output[j][i].collapsed_pattern;
				const Pattern& pattern = patterns[patternIndex];
				auto [r, g, b] = pattern.tiles[0][0].ToRGB();
				row[i][0] = b;
				row[i][1] = g;
				row[i][2] = r;
			}
			else
			{
				row[i][0] = 0;
				row[i][1] = 0;
				row[i][2] = 0;
			}
		}
	}

	cv::imshow("WFC", image);
	cv::waitKey(1);
#endif
}
boost::multi_array<WFCTileType::WFCTile, 2> WaveFunctionCollapse::GetOutput()
{
	boost::multi_array<WFCTileType::WFCTile, 2> grid;
	grid.resize(boost::extents[height][width]);

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			int patternIndex = *output[j][i].collapsed_pattern;
			const Pattern& pattern = patterns[patternIndex];

			grid[j][i] = patterns[patternIndex].tiles[0][0].tile;
		}
	}

	return grid;
}

boost::multi_array<WFCTileType::WFCTile, 2> WaveFunctionCollapse::GetOutputDuring()
{
	boost::multi_array<WFCTileType::WFCTile, 2> grid;
	grid.resize(boost::extents[height][width]);

	for (int j = 0; j < height; j++)
	{
		for (int i = 0; i < width; i++)
		{
			if (output[j][i].collapsed_pattern)
			{
				int patternIndex = *output[j][i].collapsed_pattern;
				const Pattern& pattern = patterns[patternIndex];
				grid[j][i] = pattern.tiles[0][0].tile;
			}
			else
			{
				const Pattern& pattern = patterns[DRandom::instance->GetInt(0, patterns.size() - 1)];
				grid[j][i] = pattern.tiles[0][0].tile;
			}
		}
	}

	return grid;
}
}