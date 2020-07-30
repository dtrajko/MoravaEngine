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

	TerrainTypes waterDeep;
	waterDeep.name = "Water Deep";
	waterDeep.height = 0.3f;
	waterDeep.color = glm::vec4(40.0f / 255.0f, 74.0f / 255.0f, 140.0f / 255.0f, 1.0f);
	m_Regions.push_back(waterDeep);

	TerrainTypes waterShallow;
	waterShallow.name = "Water Shallow";
	waterShallow.height = 0.6f;
	waterShallow.color = glm::vec4(54.0f / 255.0f, 102.0f / 255.0f, 197.0f / 255.0f, 1.0f);
	m_Regions.push_back(waterShallow);

	TerrainTypes sand;
	sand.name = "Sand";
	sand.height = 0.7f;
	sand.color = glm::vec4(207.0f / 255.0f, 209.0f / 255.0f, 127.0f / 255.0f, 1.0f);
	m_Regions.push_back(sand);

	TerrainTypes grass;
	grass.name = "Grass";
	grass.height = 0.8f;
	grass.color = glm::vec4(87.0f / 255.0f, 151.0f / 255.0f, 22.0f / 255.0f, 1.0f);
	m_Regions.push_back(grass);

	TerrainTypes grass2;
	grass2.name = "Grass 2";
	grass2.height = 0.85f;
	grass2.color = glm::vec4(62.0f / 255.0f, 107.0f / 255.0f, 20.0f / 255.0f, 1.0f);
	m_Regions.push_back(grass2);

	TerrainTypes rock;
	rock.name = "Rock";
	rock.height = 0.9f;
	rock.color = glm::vec4(92.0f / 255.0f, 69.0f / 255.0f, 63.0f / 255.0f, 1.0f);
	m_Regions.push_back(rock);

	TerrainTypes rock2;
	rock2.name = "Rock 2";
	rock2.height = 0.95f;
	rock2.color = glm::vec4(75.0f / 255.0f, 60.0f / 255.0f, 57.0f / 255.0f, 1.0f);
	m_Regions.push_back(rock2);

	TerrainTypes snow;
	snow.name = "Snow";
	snow.height = 1.0f;
	snow.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_Regions.push_back(snow);

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
