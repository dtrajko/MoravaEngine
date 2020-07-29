#pragma once


class TerrainSL
{
public:
	TerrainSL();
	~TerrainSL();

private:
	float m_Frequency;
	float m_Amplitude;
	float m_Lacunarity;
	float m_Persistance;
	size_t m_Octaves;
};
