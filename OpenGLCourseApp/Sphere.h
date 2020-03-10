#pragma once

#include <GL/glew.h>

#include "Mesh.h"


class Sphere : public Mesh
{
public:
	Sphere();
	
	void GenerateGeometry();
	void CreateMesh();
	virtual void RenderMesh() override;
	virtual void ClearMesh() override;
	virtual ~Sphere() override;

	float* m_Vertices;
	unsigned int* m_Indices;
	unsigned int m_VertexCount;
};
