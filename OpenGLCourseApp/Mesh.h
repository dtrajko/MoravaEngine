#pragma once

#include <GL/glew.h>


class Mesh
{
public:
	Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices);
	void RenderMesh();
	void ClearMesh();

	~Mesh();

private:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	GLsizei indexCount;

};
