#pragma once

#include "Texture.h"

#include <vector>


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MapGenerator
{
public:
	MapGenerator();
	MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, int seed, float noiseScale, glm::vec2 offset);
	~MapGenerator();

private:
	void GenerateMap();
	void Validate();

public:
	struct TerrainTypes {
		float height;
		glm::vec4 color;
		const char* name;
	};

	std::vector<TerrainTypes> m_Regions;

	enum class DrawMode
	{
		NoiseMap,
		ColorMap,
	};

	DrawMode m_DrawMode;

private:
	int m_MapWidth;
	int m_MapHeight;
	float m_NoiseScale;

	float** m_NoiseMap;
	glm::vec4* m_ColorMap;

	Texture* m_Texture;
	const char* m_FileLocation;

	unsigned int m_Octaves;
	float m_Persistance;
	float m_Lacunarity;

	int m_Seed;
	glm::vec2 m_Offset;

};
