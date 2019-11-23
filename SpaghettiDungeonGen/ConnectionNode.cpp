#include "ConnectionNode.h"

ConnectionNode::~ConnectionNode()
{
}

bool ConnectionNode::operator==(const ConnectionNode& other) const
{
	return this == &other;
}
