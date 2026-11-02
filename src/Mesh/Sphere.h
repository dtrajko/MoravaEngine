#pragma once

#include "Mesh/Mesh.h"

#include <GL/glew.h>

#include <vector>


class Sphere : public Mesh
{
public:
	Sphere();
	Sphere(glm::vec3 scale);
	virtual void Generate(glm::vec3 scale) override;
	inline void SetRadius(float radius) { m_Radius = radius; };
	inline void SetSectorCount(unsigned int sectorCount) { m_SectorCount = sectorCount; };
	inline void SetStackCount(unsigned int stackCount) { m_StackCount = stackCount; };
	inline void SetSmooth(bool smooth) { m_Radius = smooth; };
	virtual ~Sphere() override;

private:
	float m_Radius;
	float m_RadiusPrev;
	unsigned int m_SectorCount;
	unsigned int m_StackCount;
	bool m_Smooth;
};
