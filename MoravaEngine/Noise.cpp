#include "Noise.h"


Noise::Noise()
{
	m_Seed = std::rand();
	m_PerlinNoise = new PerlinNoise(m_Seed);
}

std::vector<std::vector<float>> Noise::GenerateNoiseMap(int mapWidth, int mapHeight, float scale)
{
	if (scale <= 0)
		scale = 0.0001f;

	std::vector<std::vector<float>> noiseMap = std::vector<std::vector<float>>();

	noiseMap.resize(mapWidth);
	for (int x = 0; x < mapWidth; x++) {
		noiseMap[x].resize(mapHeight);
		for (int y = 0; y < mapHeight; y++) {
			float sampleX = x / scale;
			float sampleY = y / scale;

			float perlinValue = (float)m_PerlinNoise->noise(sampleX, sampleY, 0);
			// printf("Noise::GenerateNoiseMap [ %.2ff, %.2ff ] %.2ff\n", sampleX, sampleY, perlinValue);
			// noiseMap.at(y).at(x) = perlinValue;
			noiseMap[x][y] = perlinValue;
		}
	}

	return noiseMap;
}

Noise::~Noise()
{
}
