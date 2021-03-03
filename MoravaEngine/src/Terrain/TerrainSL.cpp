#include "Terrain/TerrainSL.h"


TerrainSL::TerrainSL() : TerrainVoxel()
{
}

TerrainSL::TerrainSL(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail) : TerrainVoxel()
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

    m_MapGenerator = new MapGenerator(mapGenConf.heightMapFilePath, mapGenConf.colorMapFilePath);
    m_MapGenerator->Generate(mapGenConf, (float)m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = (float)m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

    Generate();
}

TerrainSL::~TerrainSL()
{
	Release();
    delete m_MapGenerator;
}

void TerrainSL::Generate(glm::vec3 scale)
{
	Release();

	for (int x = 0; x < (int)m_Scale.x; x++) {
		for (int y = -(int)(m_HeightMapMultiplier / 2); y < (int)(m_HeightMapMultiplier / 2 + 1); y++) {
			for (int z = 0; z < (int)m_Scale.z; z++) {

				float isoSurfaceHeight = y * (1.0f / m_Scale.y);
				glm::vec4 isoSurfaceColor = m_MapGenerator->GetRegionColor(isoSurfaceHeight + 0.5f);
				float heightFinal = m_MapGenerator->m_NoiseMap[x][z];

				if (heightFinal <= m_SeaLevel) {
					heightFinal = m_SeaLevel;
				}

				if (heightFinal >= isoSurfaceHeight + 0.5f) {
					Voxel* voxel = new Voxel();
					int voxelPositionX = x - ((int)m_Scale.x + 1) / 2;
					if ((int)m_Scale.x % 2 != 0) voxelPositionX += 1;
					int voxelPositionZ = z - ((int)m_Scale.z + 1) / 2;
					if ((int)m_Scale.z % 2 != 0) voxelPositionZ += 1;

					int voxelPositionY = (int)(isoSurfaceHeight * m_HeightMapMultiplier);
					voxel->position = glm::ivec3(voxelPositionX, voxelPositionY, voxelPositionZ);
					voxel->color = isoSurfaceColor;
					voxel->textureID = -1; // no texture
					m_Voxels.insert(std::make_pair(GetVoxelMapKey(voxel->position), voxel));
				}
			}
		}
	}
}

void TerrainSL::Update(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail)
{
    m_HeightMapMultiplier = heightMapMultiplier;
    m_IsRequiredMapRebuild = isRequiredMapRebuild;

	m_SeaLevel = seaLevel;

    m_MapGenerator->Generate(mapGenConf, (float)m_HeightMapMultiplier, m_IsRequiredMapRebuild, seaLevel, levelOfDetail);

	m_Scale.x = (float)mapGenConf.mapChunkSize;
	m_Scale.y = (float)m_HeightMapMultiplier;
	m_Scale.z = (float)mapGenConf.mapChunkSize;

	Generate();
}
