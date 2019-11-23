#include "ConnectionNodeWrapper.h"
#include <assert.h>
#include <iostream>

ConnectionNodeWrapper::ConnectionNodeWrapper(int id_in, ConnectionNode& node_in) : node_ref(std::ref(node_in))
{
	id = id_in;
}

ConnectionNodeWrapper& ConnectionNodeWrapper::operator=(const ConnectionNodeWrapper& other)
{
	this->id = other.id;
	this->node_ref = std::ref(other.node_ref.get());
	return *this;
}

bool ConnectionNodeWrapper::operator==(const ConnectionNodeWrapper& other) const
{
	return id == other.id;
}

bool ConnectionNodeWrapper::operator!=(const ConnectionNodeWrapper& other) const
{
	return id != other.id;
}

bool ConnectionNodeWrapper::operator<(const ConnectionNodeWrapper & other) const
{
	return id < other.id;
}

ConnectionNode& ConnectionNodeWrapper::node()
{
	return node_ref.get();
}
