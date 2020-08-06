#pragma once

#include "TerrainVoxel.h"

#include "MapGenerator.h"


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

private:
	MapGenerator* m_MapGenerator;

	float m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

};
