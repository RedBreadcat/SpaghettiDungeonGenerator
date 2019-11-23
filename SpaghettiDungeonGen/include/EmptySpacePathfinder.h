#pragma once
#include <array>
#include <iterator>
#include <unordered_map>
#include <optional>
#include "Coord.h"
#include "TileType.h"
#include "ConnectivityGrapher.h"
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/multi_array.hpp>
#include <iostream>


///https://www.boost.org/doc/libs/1_68_0/libs/graph/doc/table_of_contents.html
class Tile;

struct TileGraph
{

};

using vertex_descriptor_tg = int;

struct vertex_iterator_tg
{
	vertex_iterator_tg();
	vertex_iterator_tg(int v);
	vertex_descriptor_tg operator*();
	vertex_iterator_tg& operator++();
	vertex_iterator_tg operator++(int);
	bool operator==(vertex_iterator_tg const & rhs);
	bool operator!=(vertex_iterator_tg const & rhs);

	int value;
};

using vertices_size_type_tg = int;
using edge_descriptor_tg = std::pair<vertex_descriptor_tg, vertex_descriptor_tg>;
using out_edge_iterator_tg = std::array<edge_descriptor_tg, 4>::iterator;
using degree_size_type_tg = int;

int get_row(vertex_descriptor_tg vertex, TileGraph const & g);
int get_col(vertex_descriptor_tg vertex, TileGraph const & g);
vertex_descriptor_tg make_vertex(int row, int col, TileGraph const & g);
vertices_size_type_tg num_vertices(TileGraph const & g);
std::pair<vertex_iterator_tg, vertex_iterator_tg> vertices(TileGraph const & g);
vertex_descriptor_tg source(edge_descriptor_tg const & edge, TileGraph const &);
vertex_descriptor_tg target(edge_descriptor_tg const & edge, TileGraph const &);
bool operator==(edge_descriptor_tg const & lhs, edge_descriptor_tg const & rhs);
bool operator!=(edge_descriptor_tg const & lhs, edge_descriptor_tg const & rhs);
std::pair<out_edge_iterator_tg, out_edge_iterator_tg> out_edges(vertex_descriptor_tg vertex, TileGraph const & g);
degree_size_type_tg out_degree(vertex_descriptor_tg vertex, TileGraph const & g);

namespace boost
{
	template<>
	struct graph_traits<TileGraph>
	{
		using vertex_descriptor = vertex_descriptor_tg;
		using edge_descriptor = edge_descriptor_tg;

		using out_edge_iterator = out_edge_iterator_tg;
		using vertex_iterator = vertex_iterator_tg;

		using directed_category = boost::undirected_tag;
		using edge_parallel_category = boost::disallow_parallel_edge_tag;
		struct traversal_category :
			virtual boost::vertex_list_graph_tag,
			virtual boost::incidence_graph_tag {};

		using vertices_size_type = vertices_size_type_tg;
		using degree_size_type = degree_size_type_tg;

		using in_edge_iterator = void;
		using edge_iterator = void;
		using edges_size_type = void;

		static vertex_descriptor_tg null_vertex() { return -1; }
	};
}

template<>
struct std::iterator_traits<vertex_iterator_tg>
{
	using value_type = vertex_descriptor_tg;
	using difference_type = int;	//?
	using reference = vertex_iterator_tg&;	//?
	using pointer = vertex_iterator_tg*;	//?
	using iterator_category = random_access_iterator_tag;	//?
};

//Template specialisation of put() to work with std::unordered_map rather than the usually array-style. I do this because I want to store predecessors in an efficient way.
//Definition in boost/property_map/property_map.hpp
//Don't turn the ptrdiff into a vertex_descriptor_tg, since it won't compile under x64. The pointerdiff changes size, yet the size of vertex_descriptor_tg (which is an int) doesn't change.
template <>
inline void put(std::unordered_map<vertex_descriptor_tg, vertex_descriptor_tg>* storage, std::ptrdiff_t index, const vertex_descriptor_tg& value);

struct PathToNearestConnectable
{
	TileType connectedTileType;
	std::vector<Coord> path;
	ConnectionNode& node;
	int index_of_connectable;	//Door index if type==door. Open connection index if type==open connection. Otherwise undefined.
};

class EmptySpacePathfinder
{
private:
	struct NodeFoundExitBFS {};
	struct ConnectableTileReturnData
	{
		vertex_descriptor_cg last_vertex_in_path;
		vertex_descriptor_cg vertex_with_connectable;
		ConnectionNode& node;
		TileType type;
	};

public:
	class get_path : public boost::default_bfs_visitor
	{
	public:
		get_path(vertex_descriptor_tg t) : target_tile_node(t) { }
		using event_filter = boost::on_discover_vertex;

		void operator()(vertex_descriptor_tg v, const TileGraph &g) const;
	private:
		vertex_descriptor_tg target_tile_node;
	};

	class get_nearest_connectable_tile : public boost::default_bfs_visitor
	{
	public:
		get_nearest_connectable_tile(const std::vector<std::reference_wrapper<ConnectionNode>>& island_to_connect_to_in) : island_to_connect_to(island_to_connect_to_in) { }	//Find the nearest door/corridor/openconnectable that doesn't belong to the room we start in
		using event_filter = boost::on_discover_vertex;

		void operator()(vertex_descriptor_tg v, const TileGraph &g) const;
		void CheckConnectable(vertex_descriptor_tg v, vertex_descriptor_tg tile_to_check) const;
	private:
		const std::vector<std::reference_wrapper<ConnectionNode>>& island_to_connect_to;
	};

	std::optional<std::vector<Coord>> GetPath(Coord start, Coord end);
	std::optional<PathToNearestConnectable> GetPathToNearestConnectable(Coord start, const std::vector<std::reference_wrapper<ConnectionNode>>& island_to_connect_to);

private:
	TileGraph g;
};

