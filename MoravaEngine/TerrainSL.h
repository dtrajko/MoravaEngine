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
	TerrainSL(MapGenerator::MapGenConf mapGenConf, float m_HeightMapMultiplier, bool isRequiredMapRebuild);
	~TerrainSL();

	void Update(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild);
	inline MapGenerator* GetMapGenerator() { return m_MapGenerator; };

private:
	MapGenerator* m_MapGenerator;

	float m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;

};
