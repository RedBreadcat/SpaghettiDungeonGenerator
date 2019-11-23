#include "ConnectivityGraphVertexIteratorWrapper.h"
#include "Map.h"

extern Map m;

ConnectivityGraphVertexIteratorWrapper::ConnectivityGraphVertexIteratorWrapper()
{
}

ConnectivityGraphVertexIteratorWrapper::ConnectivityGraphVertexIteratorWrapper(list_iter iter_in)
{
	iter = iter_in;
}

int ConnectivityGraphVertexIteratorWrapper::operator*()
{
	ConnectionNode& node = *iter->get();
	return m.NodeToWrapperInt(node);
}

bool ConnectivityGraphVertexIteratorWrapper::operator!=(ConnectivityGraphVertexIteratorWrapper const & rhs)
{
	return iter != rhs.iter;
}

bool ConnectivityGraphVertexIteratorWrapper::operator==(ConnectivityGraphVertexIteratorWrapper const & rhs)
{
	return iter == rhs.iter;
}

ConnectivityGraphVertexIteratorWrapper& ConnectivityGraphVertexIteratorWrapper::operator++()		//Pre-increment.
{
	iter++;
	return *this;
}

ConnectivityGraphVertexIteratorWrapper ConnectivityGraphVertexIteratorWrapper::operator++(int)		//Post-increment
{
	ConnectivityGraphVertexIteratorWrapper copy(iter);	//Post-increment returns copy
	iter++;
	return copy;
}