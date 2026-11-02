#pragma once

#include "Mesh/VertexTBN.h"

#include <GL/glew.h>

#include <vector>


class MeshData
{
public:
	static unsigned int vertexStrideCount;
	static unsigned int vertexCount;
	static unsigned int indexCount;
	static std::vector<float> vertices;
	static std::vector<unsigned int> indices;

	static unsigned int floorVertexCount;
	static unsigned int floorIndexCount;
	static std::vector<float> floorVertices;
	static std::vector<unsigned int> floorIndices;

	/* Basic Quad mesh */
	static unsigned int quadVertexCount;
	static unsigned int quadIndexCount;
	static std::vector<float> quadVertices;
	static std::vector<unsigned int> quadIndices;
};
