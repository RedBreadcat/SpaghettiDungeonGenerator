#pragma once

struct Coord
{
	int x;
	int y;
};

bool operator==(Coord const& lhs, Coord const& rhs);
Coord operator+(Coord const& lhs, Coord const& rhs);
Coord operator-(Coord const& lhs, Coord const& rhs);
Coord operator*(Coord const& lhs, int rhs);