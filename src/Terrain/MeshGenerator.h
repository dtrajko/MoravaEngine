#pragma once

#include "Mesh/MeshUnity.h"

#include <glm/glm.hpp>

#include <vector>


class MeshUnityData
{
public:
	MeshUnityData(unsigned int meshWidth, unsigned int meshHeight);
	~MeshUnityData();

	void AddTriangle(int a, int b, int c);
	MeshUnity* CreateMesh();
	void Release();

public:
	std::vector<glm::vec3> m_Vertices;
	std::vector<int> m_Triangles;
	std::vector<glm::vec2> m_UVs;
	std::vector<glm::vec3> m_Normals;

	// for debug reasons
	std::vector<glm::vec2> m_XY;

	unsigned int m_VerticeIndex;
	unsigned int m_TriangleIndex;

	MeshUnity* m_Mesh;
};

/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MeshGenerator
{
public:
	MeshGenerator();
	~MeshGenerator();

	static MeshUnityData* GenerateTerrainMesh(float** heightMap, unsigned int width, unsigned int height, float heightMapMultiplier, float seaLevel, int levelOfDetail);

};
