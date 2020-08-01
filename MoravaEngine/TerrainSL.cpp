#include "TerrainSL.h"


TerrainSL::TerrainSL()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel);
}

TerrainSL::~TerrainSL()
{
    delete m_MapGenerator;
}

void TerrainSL::Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel);
}
