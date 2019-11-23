#pragma once
#include <list>
#include <memory>
#include "PlacedRoom.h"
#include "Corridor.h"

template<class T>
class ConnectionNodeFilteredIterator
{
public:
	using list_iter = std::list<std::unique_ptr<ConnectionNode>>::iterator;

	ConnectionNodeFilteredIterator(list_iter element_in);
	ConnectionNodeFilteredIterator operator++();
	bool operator==(const ConnectionNodeFilteredIterator& other);
	bool operator!=(const ConnectionNodeFilteredIterator& other);
	T& operator*();

private:
	list_iter element;
};

