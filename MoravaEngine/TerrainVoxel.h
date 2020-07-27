#pragma once

#include "PerlinNoise/PerlinNoise.hpp"
#include "TerrainBase.h"

#include <glm/glm.hpp>


class TerrainVoxel : public TerrainBase
{
public:
	TerrainVoxel();
	TerrainVoxel(glm::vec3 scale, float noiseFactor, float threshold);
	virtual ~TerrainVoxel();

	float Perlin3D(float x, float y, float z);
	void Generate();
	unsigned int GetPositionsSize();

public:
	struct Voxel {
		glm::vec3 position;
		glm::vec4 color;
		int textureID;
	};

	glm::vec3 m_Scale;
	std::vector<Voxel> m_Voxels;
	siv::PerlinNoise* m_PerlinNoise;
	float m_NoiseFactor;
	float m_NoiseThreshold;

};
