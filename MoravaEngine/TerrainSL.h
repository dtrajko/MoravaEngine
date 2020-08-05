#pragma once

#include "TerrainVoxel.h"

#include "MapGenerator.h"


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class TerrainSL : public TerrainVoxel
{
public:
	TerrainSL();
	TerrainSL(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	~TerrainSL();

	virtual void Generate(glm::vec3 scale = glm::vec3(1.0f)) override;

	void Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

private:
	MapGenerator* m_MapGenerator;

	float m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

};
