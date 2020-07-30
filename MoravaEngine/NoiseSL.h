#pragma once

#include "PerlinNoise/PerlinNoise.hpp"

#include <vector>


class NoiseSL
{
public:
	NoiseSL();
	~NoiseSL();

	static float** GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity);

public:
	static siv::PerlinNoise s_PerlinNoise;

	static float s_NoiseHeightTempMax;
	static float s_NoiseHeightTempMin;

	static float s_NoiseHeightMax;
	static float s_NoiseHeightMin;
};
