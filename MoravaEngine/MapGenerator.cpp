#include "MapGenerator.h"

#include "Log.h"

#include "NoiseSL.h"


MapGenerator::MapGenerator()
{
}

MapGenerator::~MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	m_NoiseMap = NoiseSL::GenerateNoiseMap(m_MapWidth, m_MapHeight, m_NoiseScale);

	m_Texture = new Texture("Textures/Noise/noise_001.png", m_MapWidth, m_MapHeight, true);

	int normValueMin =  10000;
	int normValueMax = -10000;

	for (int y = 0; y < m_MapHeight; y++) {
		for (int x = 0; x < m_MapWidth; x++) {

			// Convert values from [-1...1] to [0-255] range
			int normValue = (int)((((m_NoiseMap[x][y] - NoiseSL::s_ValueMin) * (255.0f - 0.0f)) / (NoiseSL::s_ValueMax - NoiseSL::s_ValueMin)) + 0.0f);

			if (normValue < normValueMin) normValueMin = normValue;
			if (normValue > normValueMax) normValueMax = normValue;

			m_Texture->SetPixel(x, y, glm::ivec4(normValue, normValue, normValue, 255));
		}
	}

	Log::GetLogger()->info("MapGenerator::GenerateMap NoiseMap Value Range [ {0} - {1} ]", NoiseSL::s_ValueMin, NoiseSL::s_ValueMax);
	Log::GetLogger()->info("MapGenerator::GenerateMap NoiseMap Normalized Value Range [ {0} - {1} ]", normValueMin, normValueMax);

	m_Texture->Save();
}
