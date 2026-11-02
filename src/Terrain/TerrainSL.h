#pragma once

#include "Terrain/MapGenerator.h"
#include "Terrain/TerrainVoxel.h"


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class TerrainSL : public TerrainVoxel
{
public:
	TerrainSL();
	TerrainSL(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	~TerrainSL();

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;

	void Update(MapGenerator::MapGenConf mapGenConf, int heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

private:
	MapGenerator* m_MapGenerator;

	int m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

};
