#pragma once

#include "PerlinNoise.h"

#include <vector>


class Noise
{
public:
	Noise();
	std::vector<std::vector<float>> GenerateNoiseMap(int mapWidth, int mapHeight, float scale);
	~Noise();

private:
	int m_Seed;
	PerlinNoise* m_PerlinNoise;
};
