#pragma once
#include <list>
#include <memory>
#include "ConnectionNodeWrapper.h"

class ConnectivityGraphVertexIteratorWrapper
{
public:
	using list_iter = std::list<std::unique_ptr<ConnectionNode>>::iterator;

	ConnectivityGraphVertexIteratorWrapper();
	ConnectivityGraphVertexIteratorWrapper(list_iter iter_in);

	int operator*();
	bool operator!=(ConnectivityGraphVertexIteratorWrapper const & rhs);
	bool operator==(ConnectivityGraphVertexIteratorWrapper const & rhs);
	ConnectivityGraphVertexIteratorWrapper& operator++();
	ConnectivityGraphVertexIteratorWrapper operator++(int);

	list_iter iter;
};

