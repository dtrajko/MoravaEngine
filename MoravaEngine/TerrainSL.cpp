#include "TerrainSL.h"


TerrainSL::TerrainSL() : TerrainVoxel()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail) : TerrainVoxel()
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

    m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

    Generate();
}

TerrainSL::~TerrainSL()
{
    delete m_MapGenerator;
}

void TerrainSL::Generate()
{
	m_Voxels.clear();

	for (int x = 0; x < m_Scale.x; x++) {
		for (int y = (int)(m_SeaLevel * m_HeightMapMultiplier); y < (int)m_Scale.y; y++) {
			for (int z = 0; z < m_Scale.z; z++) {

				float isoSurfaceHeight = y * (1.0f / m_Scale.y);
				glm::vec4 isoSurfaceColor = m_MapGenerator->GetRegionColor(isoSurfaceHeight);
				float heightFinal = m_MapGenerator->m_NoiseMap[x][z];

				if (heightFinal <= m_SeaLevel) {
					heightFinal = m_SeaLevel;
				}

				if (heightFinal >= isoSurfaceHeight) {
					Voxel voxel;
					voxel.position = glm::vec3(x - m_Scale.x / 2.0f + 0.5f, isoSurfaceHeight * m_HeightMapMultiplier, z - m_Scale.z / 2.0f + 0.5f);
					// voxel.color = m_MapGenerator->m_ColorMap[z * m_MapGenerator->m_MapGenConf.mapChunkSize + x];
					voxel.color = isoSurfaceColor;
					voxel.textureID = -1; // no texture
					m_Voxels.push_back(voxel);
				}
			}
		}
	}
}

void TerrainSL::Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

    m_MapGenerator->Generate(mapGenConf, m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

	Generate();
}
