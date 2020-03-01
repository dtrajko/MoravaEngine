#pragma once

#include <GL/glew.h>


class Tile2D
{
public:
	Tile2D();

	void CreateMesh(GLfloat* vertices, unsigned int vertexCount);
	void RenderMesh();
	void ClearMesh();
	~Tile2D();

private:
	GLuint VAO;
	GLuint VBO;
	GLsizei m_VertexCount;

};
