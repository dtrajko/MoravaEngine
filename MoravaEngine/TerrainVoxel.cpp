#include "TerrainVoxel.h"


TerrainVoxel::TerrainVoxel()
{
	m_Scale = glm::vec3(60, 24, 60);
	m_NoiseFactor = 0.05f;
	m_NoiseThreshold = 0.0f;
	m_PerlinNoise = new siv::PerlinNoise();

	Generate();
}

TerrainVoxel::TerrainVoxel(glm::vec3 scale, float noiseFactor, float threshold)
{
	m_Scale = scale;
	m_NoiseFactor = noiseFactor;
	m_NoiseThreshold = threshold;
	m_PerlinNoise = new siv::PerlinNoise();

	Generate();
}

void TerrainVoxel::Generate()
{
	for (int x = 0; x < m_Scale.x; x++) {
		for (int y = 0; y < m_Scale.y; y++) {
			for (int z = 0; z < m_Scale.z; z++) {
				if (Perlin3D(x * m_NoiseFactor, y * m_NoiseFactor, z * m_NoiseFactor) >= m_NoiseThreshold) {
					m_Positions.push_back(glm::vec3(x - m_Scale.x / 2.0f, y, z - m_Scale.z / 2.0f));
				}
			}
		}
	}
}

unsigned int TerrainVoxel::GetPositionsSize()
{
	return (unsigned int)m_Positions.size();
}

TerrainVoxel::~TerrainVoxel()
{
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
