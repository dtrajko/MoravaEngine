#pragma once

#include "Texture.h"


class MapGenerator
{
public:
	MapGenerator();
	MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, float noiseScale);
	~MapGenerator();

private:
	void GenerateMap();

private:
	int m_MapWidth;
	int m_MapHeight;
	float m_NoiseScale;

	float** m_NoiseMap;

	Texture* m_Texture;
	const char* m_FileLocation;

	unsigned int m_Octaves;
	float m_Persistance;
	float m_Lacunarity;

};
