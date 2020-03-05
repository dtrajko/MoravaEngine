#pragma once

#include <GL/glew.h>


class Mesh
{
public:
	Mesh();

	virtual void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int vertexCount, unsigned int indexCount);
	virtual void RenderMesh();
	virtual void ClearMesh();

	static void CalcAverageNormals(unsigned int* indices, unsigned int indexCount, GLfloat* vertices, unsigned int vertexCount);
	static void CalcTangentSpace(unsigned int* indices, unsigned int indexCount, GLfloat* vertices, unsigned int vertexCount);

	virtual ~Mesh();

protected:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	GLsizei m_IndexCount;

};
