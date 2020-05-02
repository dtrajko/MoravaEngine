#pragma once

#include <GL/glew.h>

#include "VertexTiling.h"


namespace MeshData
{

unsigned int vertexStrideCount = sizeof(VertexTiling) / sizeof(float);

GLfloat vertices[] =
{
	//  X      Y      Z        U     V        NX     NY     NZ        TX     TY     TZ        BX     BY     BZ       TF
	-0.5f,  0.5f, -0.5f,    1.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    1.0f,
	-0.5f, -0.5f, -0.5f,    1.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    1.0f,
	 0.5f, -0.5f, -0.5f,    0.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    1.0f,
	 0.5f,  0.5f, -0.5f,    0.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    1.0f,

	-0.5f,  0.5f,  0.5f,    0.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    1.0f,
	-0.5f, -0.5f,  0.5f,    0.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    1.0f,
	 0.5f, -0.5f,  0.5f,    1.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    1.0f,
	 0.5f,  0.5f,  0.5f,    1.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    1.0f,
	 
	 0.5f,  0.5f, -0.5f,    1.0f, 1.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    1.0f,
	 0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    1.0f,
	 0.5f, -0.5f,  0.5f,    0.0f, 0.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    1.0f,
	 0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    1.0f,

	-0.5f,  0.5f, -0.5f,    0.0f, 1.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    1.0f,
	-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    1.0f,
	-0.5f, -0.5f,  0.5f,    1.0f, 0.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    1.0f,
	-0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    1.0f,

	-0.5f,  0.5f,  0.5f,    1.0f, 1.0f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    -0.5f,  0.5f,  0.5f,    1.0f,
	-0.5f,  0.5f, -0.5f,    1.0f, 0.0f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    -0.5f,  0.5f, -0.5f,    1.0f,
	 0.5f,  0.5f, -0.5f,    0.0f, 0.0f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,     0.5f,  0.5f, -0.5f,    1.0f,
	 0.5f,  0.5f,  0.5f,    0.0f, 1.0f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,     0.5f,  0.5f,  0.5f,    1.0f,

	-0.5f, -0.5f,  0.5f,    0.0f, 1.0f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    -0.5f, -0.5f,  0.5f,    1.0f,
	-0.5f, -0.5f, -0.5f,    0.0f, 0.0f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    -0.5f, -0.5f, -0.5f,    1.0f,
	 0.5f, -0.5f, -0.5f,    1.0f, 0.0f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,     0.5f, -0.5f, -0.5f,    1.0f,
	 0.5f, -0.5f,  0.5f,    1.0f, 1.0f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,     0.5f, -0.5f,  0.5f,    1.0f,
};

unsigned int vertexCount = 15 * 4 * 6;

unsigned int indices[] =
{
	 0,  3,  1,
	 3,  2,  1,
	 4,  5,  7,
	 7,  5,  6,
	 8, 11,  9,
	11, 10,  9,
	12, 13, 15,
	15, 13, 14,
	16, 19, 17,
	19, 18, 17,
	20, 21, 23,
	23, 21, 22,
};

unsigned int indexCount = 6 * 6;

/* Floor Mesh */
GLfloat floorVertices[] =
{
	// position               tex coords      normal               tangent              bitangent               TF
	-10.0f, 0.0f, -10.0f,     0.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	 10.0f, 0.0f, -10.0f,    10.0f,  0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	-10.0f, 0.0f,  10.0f,     0.0f, 10.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	 10.0f, 0.0f,  10.0f,    10.0f, 10.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
};

unsigned int floorVertexCount = 15 * 4;

unsigned int floorIndices[] =
{
	0, 2, 1,
	1, 2, 3,
};

unsigned int floorIndexCount = 6;


/* Basic Quad mesh */
GLfloat quadVertices[] =
{
	// position            tex coords     normal               tangent              bitangent               TF
	-1.0f, 0.0f, -1.0f,    0.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	 1.0f, 0.0f, -1.0f,    1.0f, 0.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	-1.0f, 0.0f,  1.0f,    0.0f, 1.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
	 1.0f, 0.0f,  1.0f,    1.0f, 1.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,    0.0f, 0.0f, -1.0f,    1.0f,
};

unsigned int quadVertexCount = 15 * 4;

unsigned int quadIndices[] =
{
	0, 2, 1,
	1, 2, 3,
};

unsigned int quadIndexCount = 6;

}
