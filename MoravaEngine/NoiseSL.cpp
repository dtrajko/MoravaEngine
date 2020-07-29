#include "NoiseSL.h"


siv::PerlinNoise NoiseSL::s_PerlinNoise = siv::PerlinNoise();
float NoiseSL::s_ValueMin =  10000.0f;
float NoiseSL::s_ValueMax = -10000.0f;

NoiseSL::NoiseSL()
{
}

NoiseSL::~NoiseSL()
{
}

float** NoiseSL::GenerateNoiseMap(int mapWidth, int mapHeight, float scale)
{
    float** noiseMap = new float*[mapWidth];
    for (size_t i = 0; i < mapWidth; i++)
        noiseMap[i] = new float[mapHeight];

    if (scale <= 0.0f)
        scale = 0.0001f;

    float perlinValue;

    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            float sampleX = (float)x / scale;
            float sampleY = (float)y / scale;

            perlinValue = (float)s_PerlinNoise.noise2D(sampleX, sampleY);

            perlinValue = (perlinValue + 1.0f) / 2.0f;

            if (perlinValue < s_ValueMin) s_ValueMin = perlinValue;
            if (perlinValue > s_ValueMax) s_ValueMax = perlinValue;

            noiseMap[x][y] = perlinValue;
        }
    }

    return noiseMap;
}
