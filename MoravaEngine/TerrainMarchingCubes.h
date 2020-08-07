#pragma once

#include "TerrainVoxel.h"

#include "MapGenerator.h"

#include <map>


/**
 * Implementation of the marching cubes algorythm for generating voxel terrains
 */
class TerrainMarchingCubes : public TerrainVoxel
{
public:
	TerrainMarchingCubes();
	TerrainMarchingCubes(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	~TerrainMarchingCubes();

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;

	void Update(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

	void MarchingCubes();
	void ComputeSingleCube(glm::ivec3 position, int cubeSize);
	int CalculateCubeIndex(); // Sebastian Lague
	bool IsVertexAvailable(glm::ivec3 position);

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

	struct Triangle {
		glm::ivec3 point[3];
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

	std::vector<glm::ivec3> m_VertexList;

};
