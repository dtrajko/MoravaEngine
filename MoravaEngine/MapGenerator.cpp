#include "MapGenerator.h"

#include "Log.h"
#include "Math.h"
#include "NoiseSL.h"
#include "Util.h"
#include "TextureGenerator.h"


MapGenerator::MapGenerator()
{
}

MapGenerator::MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, int seed, float noiseScale, glm::vec2 offset)
{
	m_FileLocation = fileLocation;
	m_MapWidth = width;
	m_MapHeight = height;
	m_NoiseScale = noiseScale;

	m_Octaves = 3;
	m_Persistance = 1.0f;
	m_Lacunarity = 1.0f;

	m_Seed = seed;
	m_Offset = offset;

	m_Regions = std::vector<TerrainTypes>();

	TerrainTypes water;
	water.name = "Water";
	water.height = 0.4f;
	water.color = glm::vec4(70.0f / 255.0f, 114.0f / 255.0f, 210.0f / 255.0f, 1.0f);
	m_Regions.push_back(water);

	TerrainTypes land;
	land.name = "Land";
	land.height = 1.0f;
	land.color = glm::vec4(86.0f / 255.0f, 151.0f / 255.0f, 22.0f / 255.0f, 1.0f);

	m_Regions.push_back(land);

	m_DrawMode = DrawMode::ColorMap;

	GenerateMap();
}

MapGenerator::~MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	Validate();

	m_NoiseMap = NoiseSL::GenerateNoiseMap(m_MapWidth, m_MapHeight, m_Seed, m_NoiseScale, m_Octaves, m_Persistance, m_Lacunarity, m_Offset);
	m_ColorMap = new glm::vec4[m_MapWidth * m_MapHeight];

	for (int y = 0; y < m_MapHeight; y++) {
		for (int x = 0; x < m_MapWidth; x++) {
			float currentHeight = m_NoiseMap[x][y];
			for (int i = 0; i < m_Regions.size(); i++) {
				if (currentHeight <= m_Regions[i].height) {
					m_ColorMap[y * m_MapWidth + x] = m_Regions[i].color;
					break;
				}
			}
		}
	}

	if (m_DrawMode == DrawMode::NoiseMap) {
		m_Texture = TextureGenerator::TextureFromHeightMap(m_NoiseMap, m_FileLocation, m_MapWidth, m_MapHeight);
	}
	else if (m_DrawMode == DrawMode::ColorMap) {
		m_Texture = TextureGenerator::TextureFromColorMap(m_ColorMap, m_FileLocation, m_MapWidth, m_MapHeight);
	}
}

void MapGenerator::Validate()
{
	if (m_MapWidth < 1) {
		m_MapWidth = 1;
	}
	if (m_MapHeight < 1) {
		m_MapHeight = 1;
	}
	if (m_Lacunarity < 1.0f) {
		m_Lacunarity = 1.0f;
	}
	if (m_Octaves < 0) {
		m_Octaves = 0;
	}
	if (m_Persistance < 0.0f) {
		m_Persistance = 0.0f;
	}
	if (m_Persistance > 1.0f) {
		m_Persistance = 1.0f;
	}
}
