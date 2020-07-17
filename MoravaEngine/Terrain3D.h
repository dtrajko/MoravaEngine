#pragma once

#include "PerlinNoise/PerlinNoise.hpp"

#include <glm/glm.hpp>


class Terrain3D
{
public:
	Terrain3D();
	~Terrain3D();

	float Perlin3D(float x, float y, float z);
	void Generate();
	unsigned int GetCellCount();

public:
	glm::vec3 m_Scale;
	std::vector<glm::vec3> m_Positions;
	siv::PerlinNoise* m_PerlinNoise;
	float m_NoiseFactor;
	float m_NoiseThreshold;

};
