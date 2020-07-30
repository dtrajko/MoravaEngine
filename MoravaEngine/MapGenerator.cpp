#include "MapGenerator.h"

#include "Log.h"

#include "NoiseSL.h"


MapGenerator::MapGenerator()
{
}

MapGenerator::MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, int seed, float noiseScale)
{
	m_FileLocation = fileLocation;
	m_MapWidth = width;
	m_MapHeight = height;
	m_Seed = seed;
	m_NoiseScale = noiseScale;

	m_Octaves = 3;
	m_Persistance = 1.0f;
	m_Lacunarity = 1.0f;


	GenerateMap();
}

MapGenerator::~MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	m_NoiseMap = NoiseSL::GenerateNoiseMap(m_MapWidth, m_MapHeight, m_Seed, m_NoiseScale, m_Octaves, m_Persistance, m_Lacunarity);

	m_Texture = new Texture(m_FileLocation, m_MapWidth, m_MapHeight, true);

	constexpr int constValueIntMin = std::numeric_limits<int>::min();
	constexpr int constValueIntMax = std::numeric_limits<int>::max();

	int valueIntNormMin = constValueIntMax;
	int valueIntNormMax = constValueIntMin;

	for (int y = 0; y < m_MapHeight; y++) {
		for (int x = 0; x < m_MapWidth; x++) {
			// Convert values from [-1...1] to [0-255] range
			int normValue = (int)((((m_NoiseMap[x][y] - NoiseSL::s_NoiseHeightMin) * (255.0f - 0.0f)) /
				(NoiseSL::s_NoiseHeightMax - NoiseSL::s_NoiseHeightMin)) + 0.0f);

			if (normValue < valueIntNormMin) valueIntNormMin = normValue;
			if (normValue > valueIntNormMax) valueIntNormMax = normValue;

			m_Texture->SetPixel(x, y, glm::ivec4(normValue, normValue, normValue, 255));
		}
	}

	printf("MapGenerator::GenerateMap NoiseMap Value Range [%.4ff-%.4ff]\n", NoiseSL::s_NoiseHeightMin, NoiseSL::s_NoiseHeightMax);
	printf("MapGenerator::GenerateMap NoiseMap Value Range Normalized [%i-%i]\n", valueIntNormMin, valueIntNormMax);

	m_Texture->Save();
}
