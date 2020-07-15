#include "Terrain3D.h"


Terrain3D::Terrain3D()
{
	m_PerlinNoise = new siv::PerlinNoise();
	m_NoiseScale = 0.2f;

	for (int x = 0; x < 20; x++) {
		for (int y = 0; y < 20; y++) {
			for (int z = 0; z < 20; z++) {
				if (Perlin3D(x * m_NoiseScale, y * m_NoiseScale, z * m_NoiseScale) >= 0.5f) {
					printf("Terrain3D::Terrain3D Voxel position [ %i %i %i ]\n", x, y, z);
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
