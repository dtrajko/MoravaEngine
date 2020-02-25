#pragma once

#include <GL/glew.h>


class Mesh
{
public:
	Mesh();

	void CreateMesh(GLfloat* vertices, unsigned int* indices, unsigned int numOfVertices, unsigned int numOfIndices);
	void RenderMesh();
	void ClearMesh();

	static void calcAverageNormals(unsigned int* indices, unsigned int indiceCount,
		GLfloat* vertices, unsigned int verticeCount,
		unsigned int vLength, unsigned int normalOffset);

	static void calcTangentSpace(unsigned int* indices, unsigned int indiceCount,
		GLfloat* vertices, unsigned int verticeCount);

	~Mesh();

private:
	GLuint VAO;
	GLuint VBO;
	GLuint IBO;
	GLsizei indexCount;

};
