#pragma once

#include "Texture.h"


class MapGenerator
{
public:
	MapGenerator();
	~MapGenerator();

	void GenerateMap();

public:
	int m_MapWidth;
	int m_MapHeight;
	float m_NoiseScale;

	float** m_NoiseMap;

	Texture* m_Texture;

};
