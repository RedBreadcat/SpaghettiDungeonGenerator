#include "Coord.h"

bool operator==(Coord const& lhs, Coord const& rhs)
{
	return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

Coord operator+(Coord const& lhs, Coord const& rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}

Coord operator-(Coord const& lhs, Coord const& rhs)
{
	return { lhs.x - rhs.x, lhs.y - rhs.y };
}

Coord operator*(Coord const& lhs, int rhs)
{
	return { lhs.x * rhs, lhs.y * rhs };
}