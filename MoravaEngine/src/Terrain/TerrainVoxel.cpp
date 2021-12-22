#include "Terrain/TerrainVoxel.h"


TerrainVoxel::TerrainVoxel()
{
	m_Scale = glm::vec3(60, 24, 60);
	m_NoiseFactor = 0.05f;
	m_NoiseThreshold = 0.0f;
	m_PerlinNoise = new siv::PerlinNoise();
	m_Voxels = std::map<std::string, Voxel*>();
}

TerrainVoxel::TerrainVoxel(glm::vec3 scale, float noiseFactor, float threshold)
{
	m_Scale = scale;
	m_NoiseFactor = noiseFactor;
	m_NoiseThreshold = threshold;
	m_PerlinNoise = new siv::PerlinNoise();
	m_Voxels = std::map<std::string, Voxel*>();

	Generate();
}

void TerrainVoxel::Generate(glm::vec3 scale)
{
	Release();

	for (int x = 0; x < m_Scale.x; x++) {
		for (int y = 0; y < m_Scale.y; y++) {
			for (int z = 0; z < m_Scale.z; z++) {
				if (m_PerlinNoise->noise3D(x * m_NoiseFactor, y * m_NoiseFactor, z * m_NoiseFactor) >= m_NoiseThreshold) {
					Voxel* voxel = new Voxel();
					voxel->position = glm::vec3(x - m_Scale.x / 2.0f, y, z - m_Scale.z / 2.0f);
					float colorR = voxel->position.x / m_Scale.x;
					float colorG = voxel->position.y / m_Scale.y;
					float colorB = voxel->position.z / m_Scale.z;
					voxel->color = glm::vec4(1.0f - colorR, colorG, 1.0f - colorB, 0.6f);
					voxel->textureID = -1; // no texture
					m_Voxels.insert(std::make_pair(GetVoxelMapKey(voxel->position), voxel));
				}
			}
		}
	}
}

void TerrainVoxel::Release()
{
	for (auto voxel : m_Voxels)
	{
		delete voxel.second;
	}

	m_Voxels.clear();
}

bool TerrainVoxel::DeleteVoxel(glm::ivec3 position)
{
	for (auto it = m_Voxels.begin(); it != m_Voxels.end(); it++) {
		if ((*it).second->position == position) {
			it = m_Voxels.erase(it);
			return true;
		}
	}
	return false;
}

std::string TerrainVoxel::GetVoxelMapKey(glm::ivec3 position)
{
	return std::to_string(position.x) + "_" + std::to_string(position.y) + "_" + std::to_string(position.z);
}

unsigned int TerrainVoxel::GetVoxelCount()
{
	return (unsigned int)m_Voxels.size();
}

TerrainVoxel::~TerrainVoxel()
{
	Release();
	delete m_PerlinNoise;
}

float TerrainVoxel::Perlin3D(float x, float y, float z)
{
	float ab = (float)m_PerlinNoise->noise2D(x, y);
	float bc = (float)m_PerlinNoise->noise2D(y, z);
	float ac = (float)m_PerlinNoise->noise2D(x, z);

	float ba = (float)m_PerlinNoise->noise2D(y, x);
	float cb = (float)m_PerlinNoise->noise2D(z, y);
	float ca = (float)m_PerlinNoise->noise2D(z, x);

	float abc = ab + bc + ac + ba + cb + ca;
	return abc / 6.0f;
}
