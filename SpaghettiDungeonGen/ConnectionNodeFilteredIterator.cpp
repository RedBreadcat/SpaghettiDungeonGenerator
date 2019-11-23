#include "ConnectionNodeFilteredIterator.h"
#include "Map.h"

extern Map m;

template<class T>
ConnectionNodeFilteredIterator<T>::ConnectionNodeFilteredIterator(list_iter element_in)
{
	element = element_in;
	list_iter end = m.nodes.end();

	while (element != end)
	{
		if (dynamic_cast<T*>(element->get()))
		{
			break;
		}
		else
		{
			element++;
		}
	}
}

template<class T>
ConnectionNodeFilteredIterator<T> ConnectionNodeFilteredIterator<T>::operator++()
{
	list_iter end = m.nodes.end();

	while (true)
	{
		element++;
		if (element == end || dynamic_cast<T*>(element->get()))
		{
			return *this;
		}
	}
}

template<class T>
bool ConnectionNodeFilteredIterator<T>::operator==(const ConnectionNodeFilteredIterator & other)
{
	return element == other.element;
}

template<class T>
bool ConnectionNodeFilteredIterator<T>::operator!=(const ConnectionNodeFilteredIterator<T>& other)
{
	return element != other.element;
}

template<class T>
T& ConnectionNodeFilteredIterator<T>::operator*()
{
	return *(T*)element->get();
}

//Normally, templates must be defined purely in header files
//However, I like .cpp files.
//If you know which types will be used, a non-hacky workaround is possible: https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file
template class ConnectionNodeFilteredIterator<PlacedRoom>;
template class ConnectionNodeFilteredIterator<Corridor>;
