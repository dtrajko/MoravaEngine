#pragma once

#include "PerlinNoise/PerlinNoise.hpp"

#include <glm/glm.hpp>


class Terrain3D
{
public:
	Terrain3D();
	~Terrain3D();

	float Perlin3D(float x, float y, float z);
	inline std::vector<glm::vec3>* GetPositions() { return &m_Positions; };

private:
	std::vector<glm::vec3> m_Positions;
	float m_NoiseScale;
	siv::PerlinNoise* m_PerlinNoise;
};
