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
	static void Init(int seed);
	static float** GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, glm::vec2 offset);
	static void Release();

public:
	static siv::PerlinNoise s_PerlinNoise;

	static float s_NoiseHeightTempMax;
	static float s_NoiseHeightTempMin;

	static float s_NoiseHeightMax;
	static float s_NoiseHeightMin;

	static int s_RandSeed;

	static int s_MapWidth;
	static int s_MapHeight;
	static int s_Octaves;

	static float** s_NoiseMap;
	static glm::vec2** s_OctaveOffsets;

};
