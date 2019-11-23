#include "DRandom.h"
#include <iostream>

DRandom* DRandom::instance = nullptr;

DRandom::DRandom(unsigned int seed)
{
	rng.seed(seed);
	instance = this;
}

DRandom::~DRandom()
{
	instance = nullptr;
}

Direction DRandom::GetDirection()
{
	boost::random::uniform_int_distribution<int> distributionDir(0, 3);
	return static_cast<Direction>(distributionDir(rng));
}

int DRandom::GetInt(int a, int b)
{
	boost::random::uniform_int_distribution<int> dist(a, b);
	return dist(rng);
}

float DRandom::GetFloat(float a, float b)
{
	boost::random::uniform_real_distribution<float> dist(a, b);
	return dist(rng);
}

float DRandom::GetZeroOne()
{
	boost::random::uniform_real_distribution<float> dist(0.0f, 1.0f);
	return dist(rng);
}