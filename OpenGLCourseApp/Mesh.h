#pragma once

#include <GL/glew.h>


class Mesh
{
public:
	Mesh();

	virtual void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices);
	virtual void RenderMesh();
	virtual void ClearMesh();

	static void CalcAverageNormals(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount);
	static void CalcTangentSpace(unsigned int* indices, unsigned int indiceCount, GLfloat* vertices, unsigned int verticeCount);

	virtual ~Mesh();

protected:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	GLsizei indexCount;

};
