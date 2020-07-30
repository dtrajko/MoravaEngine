#pragma once

#include "TerrainVoxel.h"

#include "MapGenerator.h"


class TerrainSL : public TerrainVoxel
{
public:
	TerrainSL();
	TerrainSL(const char* fileLocation, unsigned int width, unsigned int height, float noiseScale);
	~TerrainSL();

	virtual void Generate() override;

private:
	unsigned int m_Width;
	unsigned int m_Height;
	int m_Seed;
	float m_NoiseScale;

	MapGenerator* m_MapGenerator;
	const char* m_FileLocation;

	unsigned int m_Octaves;
	float m_Persistance;
	float m_Lacunarity;

	float m_Frequency;
	float m_Amplitude;
};
