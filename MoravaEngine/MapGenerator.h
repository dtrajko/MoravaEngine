#pragma once


class MapGenerator
{
public:
	MapGenerator();
	void GenerateMap();
	~MapGenerator();

public:
	int m_MapWidth;
	int m_MapHeight;
	float m_NoiseScale;

};
