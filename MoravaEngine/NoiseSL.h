#pragma once

#include "PerlinNoise/PerlinNoise.hpp"

#include <glm/glm.hpp>

#include <vector>


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class NoiseSL
{
public:
	NoiseSL();
	~NoiseSL();

	static void Init(int seed);
	static float** GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, glm::vec2 offset);

public:
	static siv::PerlinNoise s_PerlinNoise;

	static float s_NoiseHeightTempMax;
	static float s_NoiseHeightTempMin;

	static float s_NoiseHeightMax;
	static float s_NoiseHeightMin;

	static int s_RandSeed;

};
