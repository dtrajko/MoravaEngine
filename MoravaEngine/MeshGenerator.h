#pragma once

#include "MeshUnity.h"

#include <glm/glm.hpp>

#include <vector>


class MeshData;

/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MeshGenerator
{
public:
	MeshGenerator();
	~MeshGenerator();

	static MeshData* GenerateTerrainMesh(float** heightMap, unsigned int width, unsigned int height, float heightMapMultiplier, float waterLevel = 0.5f);

};

class MeshData
{
public:
	MeshData(unsigned int meshWidth, unsigned int meshHeight);
	~MeshData();

	void AddTriangle(int a, int b, int c);
	MeshUnity* CreateMesh();

public:
	std::vector<glm::vec3> m_Vertices;
	std::vector<int> m_Triangles;
	std::vector<glm::vec2> m_UVs;
	std::vector<glm::vec3> m_Normals;

	// for debug reasons
	std::vector<glm::vec2> m_XY;

	unsigned int m_VerticeIndex;
	unsigned int m_TriangleIndex;

};
