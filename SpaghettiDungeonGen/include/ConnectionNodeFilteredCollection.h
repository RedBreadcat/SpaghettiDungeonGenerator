#pragma once
#include "ConnectionNodeFilteredIterator.h"
#include "PlacedRoom.h"
#include "Corridor.h"

template<class T>
class ConnectionNodeFilteredCollection
{
public:
	ConnectionNodeFilteredIterator<T> begin();
	ConnectionNodeFilteredIterator<T> end();
	T& recent();
	T& random();
	int size = 0;
};

