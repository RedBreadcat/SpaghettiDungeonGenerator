#include "ConnectionNodeFilteredCollection.h"
#include "Map.h"
#include "DRandom.h"

extern Map m;

template<class T>
ConnectionNodeFilteredIterator<T> ConnectionNodeFilteredCollection<T>::begin()
{
	return ConnectionNodeFilteredIterator<T>(m.nodes.begin());
}

template<class T>
ConnectionNodeFilteredIterator<T> ConnectionNodeFilteredCollection<T>::end()
{
	return ConnectionNodeFilteredIterator<T>(m.nodes.end());
}

template<class T>
T& ConnectionNodeFilteredCollection<T>::recent()
{
	return *begin();	//Doesn't simply return the first in the list. It returns the first corridor or room depending
}

template<class T>
T& ConnectionNodeFilteredCollection<T>::random()
{
	int increments = DRandom::instance->GetInt(0, size - 1);

	ConnectionNodeFilteredIterator<T> iter = begin();
	while (increments > 0)
	{
		++iter;
		increments--;
	}
	return *iter;
}

//Normally, templates must be defined purely in header files
//However, I like .cpp files.
//If you know which types will be used, a non-hacky workaround is possible: https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file
template class ConnectionNodeFilteredCollection<PlacedRoom>;
template class ConnectionNodeFilteredCollection<Corridor>;