#pragma once

#include <GL/glew.h>

#include "Mesh.h"


class Tile2D : public Mesh
{
public:
	Tile2D();

	virtual void Create(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices) override;
	virtual void Render() override;
	virtual void Clear() override;
	virtual ~Tile2D() override;


};
