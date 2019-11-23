#pragma once
#include "ConnectionNode.h"
#include <unordered_map>

//Wraps over ConnectionNode so that ConnectivityGrapher can use the integer id instead of the pointer
//Using the pointer leads to non-deterministic behaviour in Boost.Graph

class ConnectionNodeWrapper
{
public:
	ConnectionNodeWrapper(int id_in, ConnectionNode& node_in);
	ConnectionNodeWrapper& operator=(const ConnectionNodeWrapper& other);	//Must be defined because this'll be copied/moved around a vector

	bool operator==(const ConnectionNodeWrapper &other) const;
	bool operator!=(const ConnectionNodeWrapper &other) const;
	bool operator<(const ConnectionNodeWrapper &other) const;
	ConnectionNode& node();

	int id;

private:
	std::reference_wrapper<ConnectionNode> node_ref;	//References can't be reassigned, but ref_wrappers can. We need to reassign a pointer in operator=. We keep it hidden so that we don't do &wrapper.node_ref elsewhere in code and get the address of the reference_wrapper rather than what it contains
};

namespace std //Inject into namespace
{
	template <>
	struct hash<ConnectionNodeWrapper>
	{
		std::size_t operator()(const ConnectionNodeWrapper& k) const
		{
			return k.id;
		}
	};
}
