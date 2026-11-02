#pragma once

#include "Scene/Scene.h"
#include "Terrain/MapGenerator.h"
#include "Terrain/TerrainVoxel.h"

#include <map>


/**
 * Implementation of the marching cubes algorythm for generating voxel terrains
 */
class TerrainMarchingCubes : public TerrainVoxel
{
public:
	TerrainMarchingCubes();
	TerrainMarchingCubes(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail, Scene* scene);
	~TerrainMarchingCubes();

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;
	virtual void RecalculateNormals() override;
	virtual void RecalculateTangentSpace() override;

	void Update(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

	void MarchingCubes();
	bool DoesVoxelExists(glm::ivec3 position);

private:
	void CalculateVoxelRanges();
	void CalculateVoxelParameters();
	void ComputeSingleCube(glm::ivec3 position, int cubeSize);
	void GenerateVertexData();
	void GenerateDataOpenGL();
	int CalculateCubeIndex(std::vector<glm::ivec3> cubeVertices); // Sebastian Lague

public:
	struct VertexMC {
		glm::ivec3 position;
		bool inVolume;
	};
	std::vector<VertexMC*> m_VertexPositions;
	std::vector<glm::ivec3> m_EdgePositions;

private:
	struct Range {
		float min;
		float max;
	};
	std::map<std::string, Range> m_Ranges;
	void CalculateRanges(int x, int y, int z, float isoSurfaceHeight, float heightFinal, int voxelPositionX, int voxelPositionY, int voxelPositionZ);

	struct TriangleVertex {
		glm::ivec3 position;
		glm::vec4 color;
	};
	struct Triangle {
		TriangleVertex vertices[3];
		glm::vec3 normal;
	};

	std::vector<Triangle> m_Triangles;

	MapGenerator* m_MapGenerator;

	int m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

	glm::ivec3 m_VoxelRangeMin;
	glm::ivec3 m_VoxelRangeMax;

	std::vector<glm::ivec3> m_CubeVertices;
	std::vector<glm::ivec3> m_CubeEdges;
	// std::vector<glm::vec3> m_CubeNormals;

	std::vector<glm::ivec3> m_CubeEdgeIntersections;

	Scene* m_Scene;
	int m_CubeSize;

};
