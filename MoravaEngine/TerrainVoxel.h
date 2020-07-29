#pragma once

#include "PerlinNoise/PerlinNoise.hpp"
#include "TerrainBase.h"

#include <glm/glm.hpp>


class TerrainVoxel : public TerrainBase
{
public:
	struct Voxel {
		glm::vec3 position;
		glm::vec4 color;
		int textureID;
	};

	TerrainVoxel();
	TerrainVoxel(glm::vec3 scale, float noiseFactor, float threshold);
	virtual ~TerrainVoxel() override;

	virtual void Generate() override;

	float Perlin3D(float x, float y, float z);
	unsigned int GetVoxelCount();

public:
	glm::vec3 m_Scale;
	std::vector<Voxel> m_Voxels;
	siv::PerlinNoise* m_PerlinNoise;
	float m_NoiseFactor;
	float m_NoiseThreshold;

};
