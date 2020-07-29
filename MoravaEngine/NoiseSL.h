#pragma once

#include "PerlinNoise/PerlinNoise.hpp"

#include <vector>


class NoiseSL
{
public:
	NoiseSL();
	~NoiseSL();

	static float** GenerateNoiseMap(int mapWidth, int mapHeight, float scale, int octaves, float persistance, float lacunarity);

public:
	static siv::PerlinNoise s_PerlinNoise;

	static float s_ValueMin;
	static float s_ValueMax;

};
