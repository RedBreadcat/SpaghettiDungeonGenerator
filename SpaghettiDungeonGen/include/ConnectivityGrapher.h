#pragma once
#include <boost/graph/breadth_first_search.hpp>
#include <boost/shared_container_iterator.hpp>
#include <unordered_map>
#include "ConnectivityGraphVertexIteratorWrapper.h"
#include "ConnectionNodeWrapper.h"

class Map;
class Islands;

///https://www.boost.org/doc/libs/1_68_0/libs/graph/doc/table_of_contents.html

struct ConnectivityGraph
{

};

using vertex_descriptor_cg = int;
using edge_descriptor_cg = std::pair<vertex_descriptor_cg, vertex_descriptor_cg>;
using out_edge_iterator_cg = boost::shared_container_iterator<std::vector<edge_descriptor_cg>>;
using edge_iterator_cg = boost::shared_container_iterator<std::vector<edge_descriptor_cg>>;
using vertex_iterator_cg = ConnectivityGraphVertexIteratorWrapper;
using vertices_size_type_cg = int;
using edges_size_type_cg = int;
using degree_size_type_cg = int;

namespace boost
{
	template<>
	struct graph_traits<ConnectivityGraph>
	{
		using vertex_descriptor = vertex_descriptor_cg;
		using edge_descriptor = edge_descriptor_cg;

		using out_edge_iterator = out_edge_iterator_cg;
		using vertex_iterator = vertex_iterator_cg;
		using edge_iterator = edge_iterator_cg;
		using edges_size_type = edges_size_type_cg;

		using directed_category = boost::undirected_tag;
		using edge_parallel_category = boost::disallow_parallel_edge_tag;
		struct traversal_category :
			virtual boost::vertex_list_graph_tag,
			virtual boost::edge_list_graph_tag,
			virtual boost::incidence_graph_tag {};

		using vertices_size_type = vertices_size_type_cg;
		using degree_size_type = degree_size_type_cg;

		using in_edge_iterator = void;

		static vertex_descriptor_cg null_vertex() { return -1; }
	};
}

std::pair<vertex_iterator_cg, vertex_iterator_cg> vertices(ConnectivityGraph const & g);
std::pair<edge_iterator_cg, edge_iterator_cg> edges(ConnectivityGraph const & g);
edges_size_type_cg num_edges(ConnectivityGraph const & g);
vertex_descriptor_cg source(edge_descriptor_cg const & edge, ConnectivityGraph const &);
vertex_descriptor_cg target(edge_descriptor_cg const & edge, ConnectivityGraph const &);
vertices_size_type_cg num_vertices(ConnectivityGraph const & g);
std::pair<out_edge_iterator_cg, out_edge_iterator_cg> out_edges(vertex_descriptor_cg vertex, ConnectivityGraph const & g);
degree_size_type_cg out_degree(vertex_descriptor_cg vertex, ConnectivityGraph const & g);

using connection_distance_map = std::unordered_map<vertex_descriptor_cg, vertices_size_type_cg>;

class ConnectivityGrapher
{
private:
	struct NodeFoundExitBFS {};

	struct GraphvizNodePainter
	{
		void operator()(std::ostream& out, vertex_descriptor_cg v);
	};

public:
	using node_link = boost::graph_traits<ConnectivityGraph>::edge_descriptor;		//Graph edge

	class find_node_id : public boost::default_bfs_visitor
	{
	public:
		find_node_id(ConnectionNode& t) : target_connection_node(t) { }
		void discover_vertex(vertex_descriptor_cg v, const ConnectivityGraph& g) const;	//discover_vertex is called when vertex is encountered for the first time https://www.boost.org/doc/libs/1_68_0/libs/graph/doc/BFSVisitor.html
	private:
		ConnectionNode& target_connection_node;
	};

	bool AreNodesConnected(ConnectionNode& node1, ConnectionNode& node2);
	connection_distance_map GetDistancesFromNode(ConnectionNode& node, bool include_corridors);
	connection_distance_map GetDistancesFromNode(vertex_descriptor_cg node, bool include_corridors);
	std::vector<std::reference_wrapper<ConnectionNode>> GetNodesWithNeighbourCount(int desired_count);
	Islands GetIslands();
	void Export();

private:
	ConnectivityGraph g;
};
