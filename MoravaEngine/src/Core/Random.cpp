#include "Core/Random.h"


std::mt19937 Random::s_RandomEngine;
std::uniform_int_distribution<std::mt19937::result_type> Random::s_Distribution;

void Random::Init()
{
	s_RandomEngine.seed(std::random_device()());
}

int Random::Int()
{
	return (int)Float();
}

float Random::Float()
{
	return (float)s_Distribution(s_RandomEngine) / (float)GetMax();
}

uint32_t Random::GetMax() {
	return std::numeric_limits<uint32_t>::max();
}
