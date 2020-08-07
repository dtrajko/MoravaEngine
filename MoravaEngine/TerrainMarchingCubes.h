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
	TerrainMarchingCubes(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	~TerrainMarchingCubes();

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;

	void Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

	void MarchingCubes();
	int CalculateCubeIndex(); // Sebastian Lague
	bool IsVertexAvailable(glm::vec3 position);

public:
	struct VertexMC {
		glm::vec3 position;
		bool inVolume;
	};
	std::vector<VertexMC*> m_VertexPositions;

private:
	struct Range {
		float min;
		float max;
	};
	std::map<std::string, Range> m_Ranges;
	void CalculateRanges(int x, int y, int z, float isoSurfaceHeight, float heightFinal, float voxelPositionX, float voxelPositionY, float voxelPositionZ);

	MapGenerator* m_MapGenerator;

	float m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

	glm::vec3 m_VoxelRangeMin;
	glm::vec3 m_VoxelRangeMax;

	std::vector<glm::vec3> m_CubeVertices;

};
