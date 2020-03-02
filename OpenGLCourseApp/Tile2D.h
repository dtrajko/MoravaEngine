#pragma once

#include <GL/glew.h>

#include "Mesh.h"


class Tile2D : public Mesh
{
public:
	Tile2D();

	virtual void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices) override;
	virtual void RenderMesh() override;
	virtual void ClearMesh() override;
	virtual ~Tile2D() override;


};
