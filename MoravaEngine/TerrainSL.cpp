#include "TerrainSL.h"


TerrainSL::TerrainSL()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf)
{
    m_HeightMapFilePath = mapGenConf.heightMapFilePath;
    m_ColorMapFilePath = mapGenConf.colorMapFilePath;
	m_DrawMode    = mapGenConf.drawMode;
    m_Width       = mapGenConf.mapWidth;
    m_Height      = mapGenConf.mapHeight;
    m_NoiseScale  = mapGenConf.noiseScale;
    m_Octaves     = mapGenConf.octaves;
    m_Persistance = mapGenConf.persistance;
    m_Lacunarity  = mapGenConf.lacunarity;
    m_Seed        = mapGenConf.seed;
    m_Offset      = mapGenConf.offset;
    m_Regions     = mapGenConf.regions;

    Generate();
}

TerrainSL::~TerrainSL()
{
}

void TerrainSL::Generate()
{
    m_MapGenerator = new MapGenerator(m_HeightMapFilePath, m_ColorMapFilePath, m_Width, m_Height, m_Seed, m_NoiseScale, m_Offset, m_DrawMode);
}
