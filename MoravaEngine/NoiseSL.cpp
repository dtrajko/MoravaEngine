#include "NoiseSL.h"

#include "Math.h"

#include <limits>
#include <cstdlib>
#include <time.h>


siv::PerlinNoise NoiseSL::s_PerlinNoise = siv::PerlinNoise();
float NoiseSL::s_NoiseHeightTempMax;
float NoiseSL::s_NoiseHeightTempMin;
float NoiseSL::s_NoiseHeightMax;
float NoiseSL::s_NoiseHeightMin;

NoiseSL::NoiseSL()
{
}

NoiseSL::~NoiseSL()
{
}

float** NoiseSL::GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity)
{
    srand((unsigned int)time(nullptr));
    int randSeed = rand() % seed;
    s_PerlinNoise.reseed(randSeed);
    printf("NoiseSL::GenerateNoiseMap randSeed = %i\n", randSeed);

    float** noiseMap = new float*[mapWidth];
    for (size_t i = 0; i < mapWidth; i++)
        noiseMap[i] = new float[mapHeight];

    glm::vec2** octaveOffsets = new glm::vec2*[octaves];
    for (int i = 0; i < octaves; i++) {
        float offsetX = Math::ConvertRangeFloat((float)rand(), 0.0f, (float)RAND_MAX, -100000.0f, 100000.0f);
        float offsetY = Math::ConvertRangeFloat((float)rand(), 0.0f, (float)RAND_MAX, -100000.0f, 100000.0f);
        octaveOffsets[i] = new glm::vec2(offsetX, offsetY);
        printf("NoiseSL::GenerateNoiseMap octaveOffsets[%i] = [%.2ff, %.2ff]\n", i, octaveOffsets[i]->x, octaveOffsets[i]->y);
    }

    if (scale <= 0.0f)
        scale = 0.0001f;

    constexpr float valueFloatMin = std::numeric_limits<float>::min();
    constexpr float valueFloatMax = std::numeric_limits<float>::max();

    s_NoiseHeightTempMin = valueFloatMax;
    s_NoiseHeightTempMax = valueFloatMin;

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
            }

            if (noiseHeight > s_NoiseHeightTempMax) {
                s_NoiseHeightTempMax = noiseHeight;
            }
            else if (noiseHeight < s_NoiseHeightTempMin) {
                s_NoiseHeightTempMin = noiseHeight;
            }

            noiseMap[x][y] = noiseHeight;
        }
    }

    // Convert range from [-1...1] to [0...1]
    s_NoiseHeightMax = valueFloatMin;
    s_NoiseHeightMin = valueFloatMax;
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            noiseMap[x][y] = Math::InverseLerp(s_NoiseHeightTempMin, s_NoiseHeightTempMax, noiseMap[x][y]);
            if (noiseMap[x][y] > s_NoiseHeightMax) {
                s_NoiseHeightMax = noiseMap[x][y];
            }
            else if (noiseMap[x][y] < s_NoiseHeightMin) {
                s_NoiseHeightMin = noiseMap[x][y];
            }
        }
    }

    printf("NoiseSL::GenerateNoiseMap Value Range [%.4ff-%.4ff]\n", s_NoiseHeightTempMin, s_NoiseHeightTempMax);
    printf("NoiseSL::GenerateNoiseMap Value Range Normalized [%.4ff-%.4ff]\n", s_NoiseHeightMin, s_NoiseHeightMax);

    return noiseMap;
}
