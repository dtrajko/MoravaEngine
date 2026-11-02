#pragma once

#include "Mesh/Mesh.h"

#include <GL/glew.h>


class Tile2D : public Mesh
{
public:
	Tile2D();
	virtual ~Tile2D() override;

	virtual void Create(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices) override;
	virtual void Render() override;
	virtual void Clear() override;

};
