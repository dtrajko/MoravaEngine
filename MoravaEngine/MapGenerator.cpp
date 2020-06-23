#include "MapGenerator.h"

#include "Noise.h"


MapGenerator::MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	std::vector<std::vector<float>> noiseMap = Noise::GenerateNoiseMap(m_MapWidth, m_MapHeight, m_NoiseScale);
}

MapGenerator::~MapGenerator()
{
}
