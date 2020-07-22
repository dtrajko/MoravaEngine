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
	glm::vec3 m_Scale;
	std::vector<glm::vec3> m_Positions;
	siv::PerlinNoise* m_PerlinNoise;
	float m_NoiseFactor;
	float m_NoiseThreshold;

};
