#include "TerrainSL.h"


TerrainSL::TerrainSL() : TerrainVoxel()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail) : TerrainVoxel()
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = mapGenConf.mapChunkSize;
	m_Scale.y = mapGenConf.mapChunkSize;
	m_Scale.z = mapGenConf.mapChunkSize;

    Generate();
}

TerrainSL::~TerrainSL()
{
    delete m_MapGenerator;
}

void TerrainSL::Generate()
{
	for (int x = 0; x < m_Scale.x; x++) {
		// for (int y = 0; y < m_Scale.y; y++) {
			for (int z = 0; z < m_Scale.z; z++) {
				Voxel voxel;					
				voxel.position = glm::vec3(x - m_Scale.x / 2.0f, m_MapGenerator->m_NoiseMap[x][z], z - m_Scale.z / 2.0f);
				voxel.color = m_MapGenerator->m_ColorMap[z * m_MapGenerator->m_MapGenConf.mapChunkSize + x];
				voxel.textureID = -1; // no texture
				m_Voxels.push_back(voxel);
			}
		// }
	}
}

void TerrainSL::Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);
}
