#include "Terrain/NoiseSL.h"

#include "Core/Math.h"

#include <limits>
#include <cstdlib>
#include <time.h>


siv::PerlinNoise NoiseSL::s_PerlinNoise = siv::PerlinNoise();
float NoiseSL::s_NoiseHeightTempMax;
float NoiseSL::s_NoiseHeightTempMin;
float NoiseSL::s_NoiseHeightMax;
float NoiseSL::s_NoiseHeightMin;
int NoiseSL::s_RandSeed;

int NoiseSL::s_MapWidth;
int NoiseSL::s_MapHeight;
int NoiseSL::s_Octaves;

float** NoiseSL::s_NoiseMap;
glm::vec2** NoiseSL::s_OctaveOffsets;


void NoiseSL::Init(int seed)
{
    // srand((unsigned int)time(nullptr));
    s_RandSeed = 0; // rand() % seed;
    // s_PerlinNoise.reseed(s_RandSeed);
    printf("NoiseSL::Init s_RandSeed = %i\n", s_RandSeed);
}

float** NoiseSL::GenerateNoiseMap(int mapWidth, int mapHeight, int seed, float scale, int octaves, float persistance, float lacunarity, glm::vec2 offset)
{
    s_MapWidth = mapWidth;
    s_MapHeight = mapHeight;
    s_Octaves = octaves;

    s_NoiseMap = new float*[mapWidth];
    for (size_t i = 0; i < mapWidth; i++)
        s_NoiseMap[i] = new float[mapHeight];

    s_OctaveOffsets = new glm::vec2*[octaves];
    for (int i = 0; i < octaves; i++) {
        float offsetX = Math::ConvertRangeFloat((float)rand(), 0.0f, (float)RAND_MAX, -100000.0f, 100000.0f) + offset.x;
        float offsetY = Math::ConvertRangeFloat((float)rand(), 0.0f, (float)RAND_MAX, -100000.0f, 100000.0f) + offset.y;
        s_OctaveOffsets[i] = new glm::vec2(offsetX, offsetY);
        printf("NoiseSL::GenerateNoiseMap octaveOffsets[%i] = [%.2ff, %.2ff]\n", i, s_OctaveOffsets[i]->x, s_OctaveOffsets[i]->y);
    }

    if (scale <= 0.0f)
        scale = 0.0001f;

    constexpr float valueFloatMin = std::numeric_limits<float>::min();
    constexpr float valueFloatMax = std::numeric_limits<float>::max();

    s_NoiseHeightTempMin = valueFloatMax;
    s_NoiseHeightTempMax = valueFloatMin;

    float halfWidth = mapWidth / 2.0f;
    float halfHeight = mapHeight / 2.0f;

    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {

            float amplitude = 1.0f;
            float frequency = 1.0f;
            float noiseHeight = 0.0f;

            for (int i = 0; i < octaves; i++) {
                float sampleX = (x - halfWidth) / scale * frequency + s_OctaveOffsets[i]->x;
                float sampleY = (y - halfHeight) / scale * frequency + s_OctaveOffsets[i]->y;

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

            s_NoiseMap[x][y] = noiseHeight;
        }
    }

    // Convert range from [-1...1] to [0...1]
    s_NoiseHeightMax = valueFloatMin;
    s_NoiseHeightMin = valueFloatMax;
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            s_NoiseMap[x][y] = Math::InverseLerp(s_NoiseHeightTempMin, s_NoiseHeightTempMax, s_NoiseMap[x][y]);
            if (s_NoiseMap[x][y] > s_NoiseHeightMax) {
                s_NoiseHeightMax = s_NoiseMap[x][y];
            }
            else if (s_NoiseMap[x][y] < s_NoiseHeightMin) {
                s_NoiseHeightMin = s_NoiseMap[x][y];
            }
        }
    }

    printf("NoiseSL::GenerateNoiseMap Value Range [%.4ff-%.4ff]\n", s_NoiseHeightTempMin, s_NoiseHeightTempMax);
    printf("NoiseSL::GenerateNoiseMap Value Range Normalized [%.4ff-%.4ff]\n", s_NoiseHeightMin, s_NoiseHeightMax);

    return s_NoiseMap;
}

void NoiseSL::Release()
{
    for (size_t x = 0; x < s_MapWidth; x++) {
        delete[] s_NoiseMap[x];
    }
    delete[] s_NoiseMap;

    for (size_t x = 0; x < s_Octaves; x++) {
        delete[] s_OctaveOffsets[x];
    }
    delete[] s_OctaveOffsets;

}
