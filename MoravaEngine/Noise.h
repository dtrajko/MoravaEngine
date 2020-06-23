#pragma once

#include "PerlinNoise.h"

#include <vector>


class Noise
{
public:
	Noise();
	static std::vector<std::vector<float>> GenerateNoiseMap(int mapWidth, int mapHeight, float scale);
	~Noise();

private:
	static int m_Seed;
	static PerlinNoise* m_PerlinNoise;
};
