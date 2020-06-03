#include "Noise.h"


Noise::Noise()
{
}

std::vector<float> Noise::GenerateNoiseMap(int mapWidth, int mapHeight)
{
	std::vector<float> noiseMap(mapWidth * mapHeight);

	for (int y = 0; y < mapHeight; y++) {
		for (int x = 0; x < mapWidth; x++) {

		}
	}

	return noiseMap;
}

Noise::~Noise()
{
}
