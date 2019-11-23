#include "EmptySpacePathfinder.h"
#include "Map.h"
#include <boost/graph/visitors.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/connected_components.hpp>
#include <iostream>
#include <unordered_map>

using namespace std;

//https://kukuruku.co/post/masking-a-class-in-boost-graph-part-2-completing-implementation-of-concept-support/

extern Map m;

vertex_iterator_tg::vertex_iterator_tg()
{
}

vertex_iterator_tg::vertex_iterator_tg(int v)
{
	value = v;
}

vertex_descriptor_tg vertex_iterator_tg::operator*()
{
	return value;
}

vertex_iterator_tg& vertex_iterator_tg::operator++()		//Pre-increment. https://stackoverflow.com/questions/3846296/how-to-overload-the-operator-in-two-different-ways-for-postfix-a-and-prefix
{
	value++;
	return *this;
}

vertex_iterator_tg vertex_iterator_tg::operator++(int)	//Post-increment. The int is a dummy to indicate this.
{
	vertex_iterator_tg copy(value);	//Post-increment returns copy
	value++;
	return copy;
}

bool vertex_iterator_tg::operator==(vertex_iterator_tg const & rhs)
{
	return value == rhs.value;
}

bool vertex_iterator_tg::operator!=(vertex_iterator_tg const & rhs)
{
	return value != rhs.value;
}

int get_row(vertex_descriptor_tg vertex, TileGraph const & g)
{
	return vertex / (int)m.data.shape()[1];	//Divide by cols
}

int get_col(vertex_descriptor_tg vertex, TileGraph const & g)
{
	return vertex % m.data.shape()[1];	//Modulo by cols
}

vertex_descriptor_tg make_vertex(int row, int col, TileGraph const & g)
{
	return col + row * (int)m.data.shape()[1];	//do (row, col) -> 1D
}

std::pair<vertex_iterator_tg, vertex_iterator_tg> vertices(TileGraph const & g)
{
	return std::make_pair(vertex_iterator_tg(0), vertex_iterator_tg((int)m.data.num_elements() - 1));
}

vertices_size_type_tg num_vertices(TileGraph const & g)
{
	return (int)m.data.num_elements();
}

//Edge
vertex_descriptor_tg source(edge_descriptor_tg const & edge, TileGraph const &)
{
	return edge.first;
}

vertex_descriptor_tg target(edge_descriptor_tg const & edge, TileGraph const &)
{
	return edge.second;
}

bool operator==(edge_descriptor_tg const & lhs, edge_descriptor_tg const & rhs)
{
	return (lhs.first == rhs.first && lhs.second == rhs.second) || (lhs.first == rhs.second && lhs.second == rhs.first);
}

bool operator!=(edge_descriptor_tg const & lhs, edge_descriptor_tg const & rhs)
{
	return !(lhs == rhs);
}

std::array<edge_descriptor_tg, 4> out_edges_buffer;
std::pair<out_edge_iterator_tg, out_edge_iterator_tg> out_edges(vertex_descriptor_tg vertex, TileGraph const & g)
{
	int arr_size = 0;

	if (m.data.data()[vertex].IsPathable())
	{
		int row = get_row(vertex, g);
		int col = get_col(vertex, g);

		auto add_if_pathable = [&](vertex_descriptor_tg neighbour)
		{
			if (m.data.data()[neighbour].IsPathable())
			{
				out_edges_buffer[arr_size] = edge_descriptor_tg(vertex, neighbour);
				arr_size++;
			}
		};

		if (row != 0)
		{
			add_if_pathable(make_vertex(row - 1, col, g));	//Up
		}

		if (row != m.data.shape()[0] - 1)	//rows - 1
		{
			add_if_pathable(make_vertex(row + 1, col, g));	//Down
		}

		if (col != 0)
		{
			add_if_pathable(make_vertex(row, col - 1, g));	//Left
		}

		if (col != m.data.shape()[1] - 1)	//cols - 1
		{
			add_if_pathable(make_vertex(row, col + 1, g));	//Right
		}
	}

	return make_pair(out_edges_buffer.begin(), out_edges_buffer.begin() + arr_size);
}

degree_size_type_tg out_degree(vertex_descriptor_tg vertex, TileGraph const & g)
{
	out_edges(vertex, g);	//Modify the vector
	return (int)out_edges_buffer.size();	//Get the size of the vector
}

template <>
inline void put(unordered_map<vertex_descriptor_tg, vertex_descriptor_tg>* storage, std::ptrdiff_t index, const vertex_descriptor_tg& value)
{
	storage->insert({ index, value });
}

void EmptySpacePathfinder::get_path::operator()(vertex_descriptor_tg v, const TileGraph &g) const
{
	if (v == target_tile_node)
	{
		throw NodeFoundExitBFS();	//Node found. Exit BFS early. Must throw some kind of value when using event_filter, it seems.
	}
}

void EmptySpacePathfinder::get_nearest_connectable_tile::operator()(vertex_descriptor_tg v, const TileGraph &g) const
{
	int y = get_row(v, g);
	int x = get_col(v, g);

	if (y != 0)
	{
		vertex_descriptor_tg up = make_vertex(y - 1, x, g);
		CheckConnectable(v, up);
	}

	if (y != m.data.shape()[0] - 1)	//rows - 1
	{
		vertex_descriptor_tg down = make_vertex(y + 1, x, g);
		CheckConnectable(v, down);
	}

	if (x != 0)
	{
		vertex_descriptor_tg left = make_vertex(y, x - 1, g);
		CheckConnectable(v, left);
	}

	if (x != m.data.shape()[1] - 1)	//cols - 1
	{
		vertex_descriptor_tg right = make_vertex(y, x + 1, g);
		CheckConnectable(v, right);
	}
}

void EmptySpacePathfinder::get_nearest_connectable_tile::CheckConnectable(vertex_descriptor_tg v, vertex_descriptor_tg tile_to_check) const
{
	TileType tile_type = m.data.data()[tile_to_check].type;
	if (tile_type == TileType::PossibleConnection || tile_type == TileType::CorridorWall)
	{
		ConnectionNode& node = m.data.data()[tile_to_check].GetNode();

		for (ConnectionNode& acceptable_room_or_corridor : island_to_connect_to)
		{
			if (node == acceptable_room_or_corridor)
			{
				throw ConnectableTileReturnData{ v, tile_to_check, acceptable_room_or_corridor, tile_type };
			}
		}
	}
}

optional<vector<Coord>> EmptySpacePathfinder::GetPath(Coord start, Coord end)
{
	if (m.data[start.y][start.x].type != TileType::Nothing && m.data[end.y][end.x].type != TileType::Nothing)
	{
		return nullopt;	//Early exit if path is not possible. Also prevents a bug where the starting tile is impassable, yet the destination is right next to it
	}

	vertex_descriptor_tg start_vertex = make_vertex(start.y, start.x, g);
	vertex_descriptor_tg finish_vertex = make_vertex(end.y, end.x, g);

	unordered_map<vertex_descriptor_tg, vertex_descriptor_tg> predecessors;

	try
	{
		boost::breadth_first_search(
			g,
			start_vertex,
			boost::visitor(
				boost::make_bfs_visitor(
					make_pair(
						get_path(finish_vertex),
						boost::record_predecessors(&predecessors, boost::on_tree_edge())
					)
				)
			).vertex_index_map(boost::identity_property_map())
		);

		return nullopt;
	}
	catch (NodeFoundExitBFS e)
	{
		(void)e;	//Remove the unused variable warning. The reason I don't just catch (...) is because it's probably bad because then this exception handler could catch actual errors.
		vector<Coord> path;

		auto current_vertex = finish_vertex;
		while (current_vertex != start_vertex)
		{
			path.push_back({ get_col(current_vertex, g), get_row(current_vertex, g) });

			current_vertex = predecessors[current_vertex];
		}
		path.push_back({ get_col(current_vertex, g), get_row(current_vertex, g) });
		return path;
	}
}

optional<PathToNearestConnectable> EmptySpacePathfinder::GetPathToNearestConnectable(Coord start, const vector<reference_wrapper<ConnectionNode>>& island_to_connect_to)
{
	if (m.data[start.y][start.x].type != TileType::Nothing)
	{
		return nullopt;	//Early exit if path is not possible. Also prevents a bug where the starting tile is impassable, yet the destination is right next to it
	}

	vertex_descriptor_tg start_vertex = make_vertex(start.y, start.x, g);
	unordered_map<vertex_descriptor_tg, vertex_descriptor_tg> predecessors;

	try
	{
		boost::breadth_first_search(
			g,
			start_vertex,
			boost::visitor(
				boost::make_bfs_visitor(
					make_pair(
						get_nearest_connectable_tile(island_to_connect_to),
						boost::record_predecessors(&predecessors, boost::on_tree_edge())
					)
				)
			).vertex_index_map(boost::identity_property_map())
		);

		return nullopt;
	}
	catch (ConnectableTileReturnData data)	//The room or corridor the BFS found
	{
		vector<Coord> path;	//Path is from outside-door-tile to outside-door-tile.

		vertex_descriptor_tg current_vertex = data.last_vertex_in_path;
		ConnectionNode& node = data.node;

		Coord end = { get_col(data.vertex_with_connectable, g), get_row(data.vertex_with_connectable, g) };

		while (current_vertex != start_vertex)
		{
			path.push_back({ get_col(current_vertex, g), get_row(current_vertex, g) });
			current_vertex = predecessors[current_vertex];
		}
		path.push_back({ get_col(current_vertex, g), get_row(current_vertex, g) });

		if (node.IsRoom())
		{
			int connectable_index = -1;
			auto& room = static_cast<PlacedRoom&>(node);
			if (data.type == TileType::PossibleConnection)
			{
				for (size_t i = 0; i < room.possible_doors.size(); i++)
				{
					if (room.p + room.possible_doors[i].GetOutsideAdjacentTile() == end)
					{
						connectable_index = (int)i;
						break;
					}
				}

				if (connectable_index == -1)
				{
					cout << "Door not found. Room is " << &room << '\n';
					cout << "Start: " << start.x << ' ' << start.y << '\n';
					cout << "End: " << end.x << ' ' << end.y << '\n';
					cout << "Room: " << room.p.x << ' ' << room.p.y << '\n';
					cout << "RoomID: " << room.prefab_index << '\n';
					cout << "Doors:\n";
					for (size_t i = 0; i < room.possible_doors.size(); i++)
					{
						Coord outside = room.p + room.possible_doors[i].tile_outside_relative;
						Coord outside_adjacent = room.p + room.possible_doors[i].GetOutsideAdjacentTile();
						string facing = "Unknown";
						switch (room.possible_doors[i].facing)
						{
						case DoorFacing::Top:
							facing = "Top";
							break;
						case DoorFacing::Bottom:
							facing = "Bottom";
							break;
						case DoorFacing::Left:
							facing = "Left";
							break;
						case DoorFacing::Right:
							facing = "Right";
							break;
						case DoorFacing::SingleTile:
							facing = "Single";
							break;
						}
						cout << 'D' << i << ": (" << outside.x << ", " << outside.y << ") (" <<
							outside_adjacent.x << ", " << outside_adjacent.y << ") " << facing << '\n';
					}

					cout << "Path:\n";

					for (auto& p : path)
					{
						cout << p.x << " " << p.y << '\n';
					}
					m.Display(true);
					assert(false);
				}
			}
			else
			{
				assert(false);
			}
			return PathToNearestConnectable{ data.type, path, node, connectable_index };
		}
		return PathToNearestConnectable{ data.type, path, node };
	}
}