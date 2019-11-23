#pragma once
#include <boost/random.hpp>	//Same generation across platforms, compared to std? Haven't tested either.
#include "Direction.h"

class DRandom
{
public:
	DRandom(unsigned int seed);
	~DRandom();

	static DRandom* instance;

	Direction GetDirection();
	int GetInt(int a, int b);
	float GetFloat(float a, float b);	//TODO: This RNG won't be deterministic across platforms
	float GetZeroOne();					//TODO: This RNG won't be deterministic across platforms

private:
	boost::random::mt19937 rng;
};

