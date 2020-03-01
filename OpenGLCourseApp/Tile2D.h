#pragma once

#include <GL/glew.h>


class Tile2D
{
public:
	Tile2D();

	void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices);
	void RenderMesh();
	void ClearMesh();
	~Tile2D();

private:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	GLsizei indexCount;

};
