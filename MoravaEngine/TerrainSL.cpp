#include "TerrainSL.h"


TerrainSL::TerrainSL()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild);
}

TerrainSL::~TerrainSL()
{
    delete m_MapGenerator;
}

void TerrainSL::Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild);
}
