#include "MapGenerator.h"

#include "Noise.h"


MapGenerator::MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	Noise* noise = new Noise();
	std::vector<std::vector<float>> noiseMap = noise->GenerateNoiseMap(m_MapWidth, m_MapHeight, m_NoiseScale);
}

MapGenerator::~MapGenerator()
{
}
