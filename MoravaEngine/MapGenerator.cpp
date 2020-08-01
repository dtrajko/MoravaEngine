#include "MapGenerator.h"

#include "Log.h"
#include "Math.h"
#include "NoiseSL.h"
#include "Util.h"
#include "TextureGenerator.h"


MapGenerator::MapGenerator()
{
}

MapGenerator::MapGenerator(const char* heightMapFilePath, const char* colorMapFilePath, unsigned int width, unsigned int height,
	int seed, float noiseScale, glm::vec2 offset, DrawMode drawMode)
{
	m_MapGenConf.heightMapFilePath = heightMapFilePath;
	m_MapGenConf.colorMapFilePath = colorMapFilePath;
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
	waterDeep.color = glm::vec4(48.0f / 255.0f, 90.0f / 255.0f, 174.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(waterDeep);

	TerrainTypes waterShallow;
	waterShallow.name = "Water Shallow";
	waterShallow.height = 0.5f;
	waterShallow.color = glm::vec4(54.0f / 255.0f, 102.0f / 255.0f, 196.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(waterShallow);

	TerrainTypes sand;
	sand.name = "Sand";
	sand.height = 0.55f;
	sand.color = glm::vec4(208.0f / 255.0f, 210.0f / 255.0f, 128.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(sand);

	TerrainTypes grass;
	grass.name = "Grass";
	grass.height = 0.6f;
	grass.color = glm::vec4(86.0f / 255.0f, 150.0f / 255.0f, 22.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(grass);

	TerrainTypes grass2;
	grass2.name = "Grass 2";
	grass2.height = 0.7f;
	grass2.color = glm::vec4(62.0f / 255.0f, 108.0f / 255.0f, 20.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(grass2);

	TerrainTypes rock;
	rock.name = "Rock";
	rock.height = 0.8f;
	rock.color = glm::vec4(92.0f / 255.0f, 70.0f / 255.0f, 64.0f / 255.0f, 1.0f);
	m_MapGenConf.regions.push_back(rock);

	TerrainTypes rock2;
	rock2.name = "Rock 2";
	rock2.height = 0.9f;
	rock2.color = glm::vec4(75.0f / 255.0f, 60.0f / 255.0f, 58.0f / 255.0f, 1.0f);
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

	m_TextureHeightMap = TextureGenerator::TextureFromHeightMap(m_NoiseMap, m_MapGenConf.heightMapFilePath, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
	m_TextureColorMap = TextureGenerator::TextureFromColorMap(m_ColorMap, m_MapGenConf.colorMapFilePath, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);

	if (m_MapGenConf.drawMode == DrawMode::HeightMap) {}
	else if (m_MapGenConf.drawMode == DrawMode::ColorMap) {}
	else if (m_MapGenConf.drawMode == DrawMode::Mesh) {
		m_MeshData = MeshGenerator::GenerateTerrainMesh(m_NoiseMap, m_MapGenConf.mapWidth, m_MapGenConf.mapHeight);
		m_Mesh = m_MeshData->CreateMesh();
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
