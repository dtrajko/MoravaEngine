#include "NoiseSL.h"

#include <limits>


siv::PerlinNoise NoiseSL::s_PerlinNoise = siv::PerlinNoise();

constexpr float constMinValueFloat = std::numeric_limits<float>::min();
constexpr float constMaxValueFloat = std::numeric_limits<float>::max();

float NoiseSL::s_ValueMin = constMaxValueFloat;
float NoiseSL::s_ValueMax = constMinValueFloat;

NoiseSL::NoiseSL()
{
}

NoiseSL::~NoiseSL()
{
}

float** NoiseSL::GenerateNoiseMap(int mapWidth, int mapHeight, float scale, int octaves, float persistance, float lacunarity)
{
    float** noiseMap = new float*[mapWidth];
    for (size_t i = 0; i < mapWidth; i++)
        noiseMap[i] = new float[mapHeight];

    if (scale <= 0.0f)
        scale = 0.0001f;

    constexpr float maxNoiseHeight = std::numeric_limits<float>::min();
    constexpr float minNoiseHeight = std::numeric_limits<float>::max();

    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {

            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseHeight = 0.0f;

            for (int i = 0; i < octaves; i++) {
                float sampleX = (float)x / scale * frequency;
                float sampleY = (float)y / scale * frequency;

                float perlinValue = (float)s_PerlinNoise.noise2D(sampleX, sampleY);
                noiseHeight += perlinValue * amplitude;

                amplitude *= persistance;
                frequency *= lacunarity;

                // noiseHeight = (noiseHeight + 1.0f) / 2.0f;

                if (noiseHeight < s_ValueMin) s_ValueMin = noiseHeight;
                if (noiseHeight > s_ValueMax) s_ValueMax = noiseHeight;

            }

            noiseMap[x][y] = noiseHeight;
        }
    }

    printf("NoiseSL::GenerateNoiseMap Value Range [%.4ff-%.4ff]\n", s_ValueMin, s_ValueMax);

    return noiseMap;
}
