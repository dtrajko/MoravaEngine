#pragma once

#include <GL/glew.h>

#include "Mesh.h"


class Sphere : public Mesh
{
public:
	Sphere();
	
	void GenerateGeometry();
	void GenerateGeometrySongHo();
	void Create();
	virtual void Render() override;
	virtual void Clear() override;

	inline void SetRadius(float radius) { m_Radius = radius; };
	inline void SetSectorCount(unsigned int sectorCount) { m_SectorCount = sectorCount; };
	inline void SetStackCount(unsigned int stackCount) { m_StackCount = stackCount; };
	inline void SetSmooth(bool smooth) { m_Radius = smooth; };

	virtual ~Sphere() override;

	float* m_Vertices;
	unsigned int* m_Indices;
	unsigned int m_VertexCount;

	float m_Radius;
	unsigned int m_SectorCount;
	unsigned int m_StackCount;
	bool m_Smooth;
};
