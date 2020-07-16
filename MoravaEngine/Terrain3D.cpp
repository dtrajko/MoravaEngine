#include "Terrain3D.h"


Terrain3D::Terrain3D()
{
	m_Scale = glm::vec3(20, 20, 20);
	m_PerlinNoise = new siv::PerlinNoise();
	m_NoiseFactor = 0.4f;
	m_NoiseThreshold = 0.05f;

	Generate();
}

void Terrain3D::Generate()
{
	for (int x = 0; x < m_Scale.x; x++) {
		for (int y = 0; y < m_Scale.y; y++) {
			for (int z = 0; z < m_Scale.z; z++) {
				if (Perlin3D(x * m_NoiseFactor, y * m_NoiseFactor, z * m_NoiseFactor) >= m_NoiseThreshold) {
					m_Positions.push_back(glm::vec3(x, y, z));
				}
			}
		}
	}
}

Terrain3D::~Terrain3D()
{
}

float Terrain3D::Perlin3D(float x, float y, float z)
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
