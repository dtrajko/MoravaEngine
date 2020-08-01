#include "MapGenerator.h"

#include "Log.h"
#include "Math.h"
#include "NoiseSL.h"
#include "Util.h"
#include "TextureGenerator.h"


MapGenerator::MapGenerator()
{
}

MapGenerator::MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, int seed, float noiseScale, glm::vec2 offset, DrawMode drawMode)
{
	m_MapGenConf.heightMapFilePath = fileLocation;
	m_MapGenConf.colorMapFilePath = fileLocation;
	m_MapGenConf.drawMode = drawMode;
	m_MapGenConf.mapWidth = width;
	m_MapGenConf.mapHeight = height;
	m_MapGenConf.noiseScale = noiseScale;

	m_MapGenConf.octaves = 3;
	m_MapGenConf.persistance = 1.0f;
	m_MapGenConf.lacunarity = 1.0f;

	m_MapGenConf.seed = seed;
	m_MapGenConf.offset = offset;

	m_MapGenConf.regions = std::vector<TerrainTypes>();

	TerrainTypes waterDeep;
	waterDeep.name = "Water Deep";
	waterDeep.height = 0.4f;
	waterDeep.color = glm::vec4(40.0f / 255.0f, 74.0f / 255.0f, 140.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(waterDeep);

	TerrainTypes waterShallow;
	waterShallow.name = "Water Shallow";
	waterShallow.height = 0.5f;
	waterShallow.color = glm::vec4(54.0f / 255.0f, 102.0f / 255.0f, 197.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(waterShallow);

	TerrainTypes sand;
	sand.name = "Sand";
	sand.height = 0.55f;
	sand.color = glm::vec4(207.0f / 255.0f, 209.0f / 255.0f, 127.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(sand);

	TerrainTypes grass;
	grass.name = "Grass";
	grass.height = 0.6f;
	grass.color = glm::vec4(87.0f / 255.0f, 151.0f / 255.0f, 22.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(grass);

	TerrainTypes grass2;
	grass2.name = "Grass 2";
	grass2.height = 0.7f;
	grass2.color = glm::vec4(62.0f / 255.0f, 107.0f / 255.0f, 20.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(grass2);

	TerrainTypes rock;
	rock.name = "Rock";
	rock.height = 0.8f;
	rock.color = glm::vec4(92.0f / 255.0f, 69.0f / 255.0f, 63.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(rock);

	TerrainTypes rock2;
	rock2.name = "Rock 2";
	rock2.height = 0.9f;
	rock2.color = glm::vec4(75.0f / 255.0f, 60.0f / 255.0f, 57.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(rock2);

	TerrainTypes snow;
	snow.name = "Snow";
	snow.height = 1.0f;
	snow.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_MapGenConf.regions.push_back(snow);

	GenerateMap();
}

MapGenerator::~MapGenerator()
{
}

void MapGenerator::GenerateMap()
{
	Validate();

	m_NoiseMap = NoiseSL::GenerateNoiseMap(m_MapGenConf.mapWidth, m_MapGenConf.mapHeight, m_MapGenConf.seed, m_MapGenConf.noiseScale,
		m_MapGenConf.octaves, m_MapGenConf.persistance, m_MapGenConf.lacunarity, m_MapGenConf.offset);

	m_ColorMap = new glm::vec4[m_MapGenConf.mapWidth * m_MapGenConf.mapHeight];

	for (int y = 0; y < m_MapGenConf.mapHeight; y++) {
		for (int x = 0; x < m_MapGenConf.mapWidth; x++) {
			float currentHeight = m_NoiseMap[x][y];
			for (int i = 0; i < m_MapGenConf.regions.size(); i++) {
				if (currentHeight <= m_MapGenConf.regions[i].height) {
					m_ColorMap[y * m_MapGenConf.mapWidth + x] = m_MapGenConf.regions[i].color;
					break;
				}
			}
		}
	}

	if (m_MapGenConf.drawMode == DrawMode::NoiseMap) {
		m_Texture = TextureGenerator::TextureFromHeightMap(m_NoiseMap, m_MapGenConf.heightMapFilePath, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
	}
	else if (m_MapGenConf.drawMode == DrawMode::ColorMap) {
		m_Texture = TextureGenerator::TextureFromColorMap(m_ColorMap, m_MapGenConf.colorMapFilePath, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
	}
	else if (m_MapGenConf.drawMode == DrawMode::Mesh) {
		m_Texture = TextureGenerator::TextureFromColorMap(m_ColorMap, m_MapGenConf.colorMapFilePath, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
		m_MeshData = MeshGenerator::GenerateTerrainMesh(m_NoiseMap, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
	}
}

void MapGenerator::Validate()
{
	if (m_MapGenConf.mapWidth < 1) {
		m_MapGenConf.mapWidth = 1;
	}
	if (m_MapGenConf.mapHeight < 1) {
		m_MapGenConf.mapHeight = 1;
	}
	if (m_MapGenConf.lacunarity < 1.0f) {
		m_MapGenConf.lacunarity = 1.0f;
	}
	if (m_MapGenConf.octaves < 0) {
		m_MapGenConf.octaves = 0;
	}
	if (m_MapGenConf.persistance < 0.0f) {
		m_MapGenConf.persistance = 0.0f;
	}
	if (m_MapGenConf.persistance > 1.0f) {
		m_MapGenConf.persistance = 1.0f;
	}
}
