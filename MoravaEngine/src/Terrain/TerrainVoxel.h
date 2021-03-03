#pragma once

#include "PerlinNoise/PerlinNoise.hpp"
#include "Terrain/TerrainBase.h"

#include <glm/glm.hpp>

#include <map>
#include <string>


class TerrainVoxel : public TerrainBase
{
public:
	struct Voxel {
		glm::ivec3 position;
		glm::vec4 color;
		int textureID;
	};

	TerrainVoxel();
	TerrainVoxel(glm::vec3 scale, float noiseFactor, float threshold);
	virtual ~TerrainVoxel() override;

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;
	virtual void Release() override;
	bool DeleteVoxel(glm::ivec3 position);
	std::string GetVoxelMapKey(glm::ivec3 position);

	float Perlin3D(float x, float y, float z);
	unsigned int GetVoxelCount();

public:
	glm::vec3 m_Scale;
	std::map<std::string, Voxel*> m_Voxels;
	siv::PerlinNoise* m_PerlinNoise;
	float m_NoiseFactor;
	float m_NoiseThreshold;

};
