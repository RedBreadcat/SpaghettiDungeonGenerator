#include "ConnectivityGrapher.h"
#include "Islands.h"
#include "Map.h"
#include <boost/graph/visitors.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/connected_components.hpp>
#include <fstream>
#include <sstream>

#ifndef GAME_COMPILE
#include <boost/graph/graphviz.hpp>
#include <opencv2/opencv.hpp>
#endif

extern Map m;

using namespace std;

pair<vertex_iterator_cg, vertex_iterator_cg> vertices(ConnectivityGraph const & g)
{
	auto begin = ConnectivityGraphVertexIteratorWrapper(m.nodes.begin());
	auto end = ConnectivityGraphVertexIteratorWrapper(m.nodes.end());
	return std::make_pair(begin, end);
}

//Only runs when graph visualiser is called, so doesn't need to be too optimised
std::pair<edge_iterator_cg, edge_iterator_cg> edges(ConnectivityGraph const & g)	//Edges won't be unique
{
	boost::shared_ptr<std::vector<edge_descriptor_cg>> edges(new std::vector<edge_descriptor_cg>());

	vertex_iterator_cg vi, vi_end;
	for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
	{
		ConnectionNode& node = m.IntToWrapperNode(*vi);
		if (node.IsRoom())
		{
			auto& room = static_cast<PlacedRoom&>(node);
			for (auto& d : room.possible_doors)
			{
				if (d.connection_opt)
				{
					edges->push_back(edge_descriptor_cg(*vi, m.NodeToWrapperInt(*d.connection_opt)));
				}
			}
		}
		else //Corridor
		{
			auto& corridor = static_cast<Corridor&>(node);

			for (auto& connection : corridor.connections)
			{
				edges->push_back(edge_descriptor_cg(*vi, m.NodeToWrapperInt(connection)));
			}
		}
	}

	//Remove duplicate and flipped edges
	if (edges->size() > 0)	//Prevent size_t underflow
	{
		for (size_t i = 0; i < edges->size() - 1; i++)
		{
			for (size_t j = i + 1; j < edges->size(); j++)
			{
				if ((edges->at(i).first == edges->at(j).first && edges->at(i).second == edges->at(j).second) || (edges->at(i).first == edges->at(j).second && edges->at(i).second == edges->at(j).first))
				{
					edges->erase(edges->begin() + j);
					j--;
				}
			}
		}
	}

	return boost::make_shared_container_range(edges);
}

edges_size_type_cg num_edges(ConnectivityGraph const & g)
{
	cout << "num_edges must be defined, however I have not implemented it" << endl;
	assert(false);
	return 0;
}

//Edge
vertex_descriptor_cg source(edge_descriptor_cg const & edge, ConnectivityGraph const &)
{
	return edge.first;
}

vertex_descriptor_cg target(edge_descriptor_cg const & edge, ConnectivityGraph const &)
{
	return edge.second;
}

vertices_size_type_cg num_vertices(ConnectivityGraph const & g)
{
	return m.rooms.size + m.corridors.size;
}

pair<out_edge_iterator_cg, out_edge_iterator_cg> out_edges(vertex_descriptor_cg vertex, ConnectivityGraph const & g)
{
	boost::shared_ptr<std::vector<edge_descriptor_cg>> edges(new std::vector<edge_descriptor_cg>());	//TODO: can I replace this with some fixed memory that's allocated once?

	ConnectionNode& node = m.IntToWrapperNode(vertex);
	if (node.IsRoom())
	{
		auto& room = static_cast<PlacedRoom&>(node);
		for (auto& d : room.possible_doors)
		{
			if (d.connection_opt)
			{
				edges->push_back(edge_descriptor_cg(vertex, m.NodeToWrapperInt(*d.connection_opt)));
			}
		}
	}
	else //Corridor
	{
		auto& corridor = static_cast<Corridor&>(node);

		for (auto connection : corridor.connections)
		{
			edges->push_back(edge_descriptor_cg(vertex, m.NodeToWrapperInt(connection)));
		}
	}

	return boost::make_shared_container_range(edges);
}

degree_size_type_cg out_degree(vertex_descriptor_cg vertex, ConnectivityGraph const & g)
{
	cout << "out_degree must be defined, however I have not implemented it" << endl;
	assert(false); //out degree returns the number of edges leaving the vertex passed in
	return 0;
}

void ConnectivityGrapher::find_node_id::discover_vertex(vertex_descriptor_cg v, const ConnectivityGraph& g) const
{
	if (m.IntToWrapperNode(v) == target_connection_node)
	{
		throw NodeFoundExitBFS();	//Exit BFS early
	}
}

bool ConnectivityGrapher::AreNodesConnected(ConnectionNode& node1, ConnectionNode& node2)
{
	//This isn't error handling. The boost-recommended way to stop a BFS early is to create an exception
	try
	{
		boost::breadth_first_search(g, m.NodeToWrapperInt(node1),
			visitor(
				find_node_id(node2)
			).vertex_index_map(boost::identity_property_map())//propmapIndex)
		);

		return false;	//If we don't exit BFS early, then a connection wasn't found
	}
	catch (NodeFoundExitBFS e)
	{
		(void)e;
		return true;
	}
}

connection_distance_map ConnectivityGrapher::GetDistancesFromNode(ConnectionNode& node, bool include_corridors)
{
	return GetDistancesFromNode(m.NodeToWrapperInt(node), include_corridors);
}

template <>
inline void put(std::unordered_map<vertex_descriptor_tg, vertex_descriptor_tg>* storage, std::ptrdiff_t index, const vertex_descriptor_tg& value)
{
	storage->insert({ index, value });
}

connection_distance_map ConnectivityGrapher::GetDistancesFromNode(vertex_descriptor_cg start_node, bool include_corridors)
{
	connection_distance_map d;

	using IndexMap = map<vertex_descriptor_cg, vertices_size_type_cg>;
	IndexMap mapIndex;
	boost::associative_property_map<IndexMap> propmapIndex(mapIndex);
	size_t i = 0;
	vertex_iterator_cg vi, vi_end;
	for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
	{
		boost::put(propmapIndex, *vi, i++);
	}

	unordered_map<vertex_descriptor_tg, vertex_descriptor_tg> predecessors;

	boost::breadth_first_search(
		g,
		start_node,
		visitor(
			make_bfs_visitor(
			record_predecessors(&predecessors, boost::on_tree_edge()))
		).vertex_index_map(propmapIndex)
	);

	for (auto p : predecessors)
	{
		vertex_descriptor_tg node_to_check = p.second;
		if (include_corridors || m.IntToWrapperNode(node_to_check).IsRoom())
		{
			d[p.first]++;
		}
		while (node_to_check != start_node)
		{
			if (include_corridors || m.IntToWrapperNode(node_to_check).IsRoom())
			{
				d[p.first]++;
			}
			node_to_check = predecessors[node_to_check];
		}

		d[p.first]--;	//Subtract one, because we don't include the very last movement
	}

	return d;
}

vector<reference_wrapper<ConnectionNode>> ConnectivityGrapher::GetNodesWithNeighbourCount(int desired_count)	//1: dead ends. 0: isolated
{
	std::vector<std::reference_wrapper<ConnectionNode>> nodes;
	pair<vertex_iterator_cg, vertex_iterator_cg> all_verts = vertices(g);

	for (auto v_it = all_verts.first; v_it != all_verts.second; v_it++)
	{
		std::pair<out_edge_iterator_cg, out_edge_iterator_cg> neighbours = out_edges(*v_it, g);

		int neighbour_count = 0;
		for (auto n_it = neighbours.first; n_it != neighbours.second; n_it++)
		{
			neighbour_count++;
		}
		
		if (neighbour_count == desired_count)
		{
			nodes.push_back(m.IntToWrapperNode(*v_it));
		}
	}
	return nodes;
}

Islands ConnectivityGrapher::GetIslands()	//Islands may contain corridors
{
	map<vertex_descriptor_cg, int> components;
	auto prop_map = boost::make_assoc_property_map(components);

	using IndexMap = map<vertex_descriptor_cg, size_t>;
	IndexMap mapIndex;
	boost::associative_property_map<IndexMap> propmapIndex(mapIndex);
	size_t i = 0;
	vertex_iterator_cg vi, vi_end;
	for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
	{
		boost::put(propmapIndex, *vi, i++);
	}
	
	int island_count = connected_components(g, prop_map, boost::vertex_index_map(propmapIndex));
	return Islands(components, island_count);
}

void ConnectivityGrapher::Export()
{
#ifndef GAME_COMPILE
	//Note:
	//This code works, but I've left it commented out because I haven't got logic to determine whether graphviz is available or not
	//It provides a nice visualisation of the final map's graph
	/*string dot_path = "C:\\Users\\Example\\Desktop\\temp_graph.txt";
	string img_path = "C:\\Users\\Example\\Desktop\\temp_graph_img.png";

	using IndexMap = map<vertex_descriptor_cg, vertices_size_type_cg>;
	IndexMap mapIndex;
	boost::associative_property_map<IndexMap> propmapIndex(mapIndex);
	size_t i = 0;
	vertex_iterator_cg vi, vi_end;
	for (boost::tie(vi, vi_end) = vertices(g); vi != vi_end; ++vi)
	{
		boost::put(propmapIndex, *vi, i++);
	}

	{
		ofstream file(dot_path);
		boost::write_graphviz(file, g, GraphvizNodePainter(), boost::default_writer(), boost::default_writer(), propmapIndex);	//https://www.graphviz.org/
	}
	
	std::stringstream command;
	command << "C:\\Users\\Example\\Downloads\\graphviz\\bin\\dot.exe -Tpng " << dot_path << " -o " << img_path;
	std::system(command.str().c_str());

	cv::Mat img = cv::imread(img_path, cv::IMREAD_COLOR);
	cv::namedWindow("Graph", cv::WINDOW_NORMAL);
	cv::moveWindow("Graph", 2560, 250);
	cv::resizeWindow("Graph", img.cols, img.rows);
	cv::imshow("Graph", img);

	boost::filesystem::remove(dot_path);
	boost::filesystem::remove(img_path);*/
#endif
}

void ConnectivityGrapher::GraphvizNodePainter::operator()(std::ostream& out, vertex_descriptor_cg v)
{
	//https://www.graphviz.org/doc/info/colors.html
	ConnectionNode& node = m.IntToWrapperNode(v);
	if (node.IsRoom())
	{
		PlacedRoom& room = static_cast<PlacedRoom&>(node);
		if (room.room_type == RoomType::Normal || room.room_type == RoomType::Procedural)
		{
			if (room.room_connectivity == PlacedRoom::RoomConnectivity::DeadEnd)
			{
				out << " [fillcolor=purple, style=filled] \n";
			}
			else if (room.room_connectivity == PlacedRoom::RoomConnectivity::Isolated)
			{
				out << " [fillcolor=pink, style=filled] \n";
			}
		}
		else
		{
			switch (room.room_type)
			{
			case RoomType::Boss:
				out << " [fillcolor=red, style=filled] \n";
				break;
			case RoomType::Start:
				out << " [fillcolor=green4, style=filled] \n";
				break;
			case RoomType::Treasure:
				out << " [fillcolor=orange, style=filled] \n";
				break;
			case RoomType::Shop:
				out << " [fillcolor=deepskyblue, style=filled] \n";
				break;
			}
		}
	}
	else if (node.IsCorridor())
	{
		out << " [fillcolor=gray, style=filled] \n";
	}
	else
	{
		out << " [fillcolor=black, style=filled] \n";
	}
}
