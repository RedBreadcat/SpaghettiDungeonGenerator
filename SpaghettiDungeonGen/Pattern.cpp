#include "Pattern.h"
#include <unordered_map>
#include <fstream>
#include <boost/functional/hash.hpp>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>

#ifndef GAME_COMPILE
#include <regex>
#include <opencv2/highgui.hpp>
#endif

namespace WFC
{
Pattern::Pattern(int n) :
	frequency(0),
	tile_hash(0)
{
	tiles.resize(boost::extents[n][n]);
}

size_t Pattern::GetTileHash() const
{
	return tile_hash;
}

void Pattern::SetTileHash()
{
	tile_hash = 0;
	int n = (int)tiles.shape()[0];
	for (int j = 0; j < n; j++)
	{
		for (int i = 0; i < n; i++)
		{
			boost::hash_combine(tile_hash, tiles[j][i].tile);
		}
	}
}

Pattern Pattern::GetRotated90() const
{
	int n = (int)tiles.shape()[0];
	Pattern p(n);

	for (int j = 0; j < n; j++)
	{
		for (int i = 0; i < n; i++)
		{
			p.tiles[j][i] = tiles[n - i - 1][j];
		}
	}

	p.SetTileHash();
	return p;
}

Pattern Pattern::GetFlipped() const
{
	int n = (int)tiles.shape()[0];
	Pattern p(n);

	for (int j = 0; j < n; j++)
	{
		for (int i = 0; i < n; i++)
		{
			p.tiles[j][i] = tiles[j][n - i - 1];
		}
	}

	p.SetTileHash();
	return p;
}

std::vector<Pattern> Pattern::LoadPatternsFromJSON(std::string_view path, int& n_out)
{
	rapidjson::Document doc;
	std::ifstream file(path.data());
	std::stringstream buffer;
	buffer << file.rdbuf();
	doc.Parse(buffer.str().c_str());

	boost::multi_array<WFCTileType::WFCTile, 2> texture;
	texture.resize(boost::extents[doc["height"].GetInt()][doc["width"].GetInt()]);

	for (int i = 0; i < texture.num_elements(); i++)
	{
		texture.data()[i] = (WFCTileType::WFCTile)doc["tiles"].GetArray()[i].GetInt();
	}
	n_out = doc["n"].GetInt();

	return ExtractPatternsFromTexture(n_out, texture);
}

std::vector<Pattern> Pattern::ExtractPatternsFromTexture(int n, const boost::multi_array<WFCTileType::WFCTile, 2>& texture)
{
	std::vector<Pattern> patterns;
	std::unordered_map<size_t, int> pattern_hash_to_index;

	int width = (int)texture.shape()[0];
	int height = (int)texture.shape()[1];
	for (int j = 0; j <= height - n; j++)
	{
		for (int i = 0; i <= width - n; i++)
		{
			Pattern p(n);

			for (int p_j = 0; p_j < n; p_j++)
			{
				for (int p_i = 0; p_i < n; p_i++)
				{
					p.tiles[p_j][p_i].tile = texture[j + p_j][i + p_i];
				}
			}
			p.SetTileHash();

			Pattern rotated90 = p.GetRotated90();
			Pattern rotated180 = rotated90.GetRotated90();
			Pattern rotated270 = rotated180.GetRotated90();
			Pattern flipped = p.GetFlipped();
			Pattern flipped90 = flipped.GetRotated90();
			Pattern flipped180 = flipped90.GetRotated90();
			Pattern flipped270 = flipped180.GetRotated90();

			AddPattern(p, patterns, pattern_hash_to_index);
			AddPattern(rotated90, patterns, pattern_hash_to_index);
			AddPattern(rotated180, patterns, pattern_hash_to_index);
			AddPattern(rotated270, patterns, pattern_hash_to_index);
			AddPattern(flipped, patterns, pattern_hash_to_index);
			AddPattern(flipped90, patterns, pattern_hash_to_index);
			AddPattern(flipped180, patterns, pattern_hash_to_index);
			AddPattern(flipped270, patterns, pattern_hash_to_index);
		}
	}

	return patterns;
}

void Pattern::AddPattern(const Pattern& pattern, std::vector<Pattern>& patterns, std::unordered_map<size_t, int>& pattern_hash_to_index)
{
	size_t hash = pattern.GetTileHash();
	auto found = pattern_hash_to_index.find(hash);
	int index;
	if (found == pattern_hash_to_index.end())
	{
		patterns.push_back(pattern);
		index = (int)patterns.size() - 1;
		pattern_hash_to_index.insert({ hash, index });
	}
	else
	{
		index = found->second;
	}
	patterns[index].frequency++;
}

void Pattern::ImageToJSON(std::string_view texture_path, std::string_view json_path, int n_force)
{
#ifndef GAME_COMPILE
	cv::Mat img = cv::imread(texture_path.data(), cv::IMREAD_COLOR);

	rapidjson::Document doc;
	doc.SetObject();
	auto& a = doc.GetAllocator();

	doc.AddMember("width", img.cols, a);
	doc.AddMember("height", img.rows, a);
	//Note: could have a "palette" variable, where I hardcode that "Wall2" must be an asteroid tile type, for example
	//Can hardcode various filenames to palettes here for easy reproduction.
	rapidjson::Value tile_array(rapidjson::Type::kArrayType);

	for (int j = 0; j < img.rows; j++)
	{
		cv::Vec3b* row = img.ptr<cv::Vec3b>(j);

		for (int i = 0; i < img.cols; i++)
		{
			uint8_t b = row[i][0];
			uint8_t g = row[i][1];
			uint8_t r = row[i][2];
			WFCTileType tile;
			tile.Set(r, g, b);
			int type = static_cast<int>(tile.tile);
			tile_array.PushBack(type, a);
		}
	}

	doc.AddMember("tiles", tile_array, a);

	int n;
	if (n_force == -1)
	{
		std::regex re(R"(\(n(\d+)\))");
		std::smatch match;
		std::string texture_path_str{ texture_path };
		std::regex_search(texture_path_str, match, re);
		if (match.length() > 1)
		{
			n = stoi(match[1]);
		}
		else
		{
			n = 3;
		}
	}
	else
	{
		n = n_force;
	}

	doc.AddMember("n", n, a);

	std::ofstream output(json_path.data());
	rapidjson::OStreamWrapper osw(output);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
	doc.Accept(writer);
#endif
}

}
