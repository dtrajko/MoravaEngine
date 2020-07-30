#include "TerrainSL.h"


TerrainSL::TerrainSL()
{
	Generate();
}

TerrainSL::TerrainSL(const char* fileLocation, unsigned int width, unsigned int height, float noiseScale)
{
    m_FileLocation = fileLocation;
    m_Width = width;
    m_Height = height;
    m_Seed = 123456;
    m_NoiseScale = noiseScale;

    Generate();
}

TerrainSL::~TerrainSL()
{
}

void TerrainSL::Generate()
{
    m_MapGenerator = new MapGenerator(m_FileLocation, m_Width, m_Height, m_Seed, m_NoiseScale);
}
