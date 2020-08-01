#pragma once

#include "Texture.h"
#include "MeshUnity.h"
#include "MeshGenerator.h"

#include <vector>


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MapGenerator
{
public:
	enum class DrawMode {
		NoiseMap,
		ColorMap,
		Mesh,
	};

public:
	MapGenerator();
	MapGenerator(const char* fileLocation, unsigned int width, unsigned int height, int seed, float noiseScale, glm::vec2 offset, DrawMode drawMode);
	~MapGenerator();

private:
	void GenerateMap();
	void Validate();

public:
	struct TerrainTypes {
		float height;
		glm::vec4 color;
		const char* name;
	};

	struct MapGenConf {
		const char* heightMapFilePath;
		const char* colorMapFilePath;
		MapGenerator::DrawMode drawMode;
		int mapWidth;
		int mapHeight;
		float noiseScale;
		int octaves;
		float persistance;
		float lacunarity;
		int seed;
		glm::vec2 offset;
		bool autoUpdate;
		std::vector<MapGenerator::TerrainTypes> regions;

		inline bool operator!=(const MapGenConf& other)
		{
			return
				mapWidth != other.mapWidth ||
				mapHeight != other.mapHeight ||
				noiseScale != other.noiseScale ||
				octaves != other.octaves ||
				persistance != other.persistance ||
				lacunarity != other.lacunarity ||
				seed != other.seed ||
				offset.x != other.offset.x ||
				offset.y != other.offset.y ||
				autoUpdate != other.autoUpdate ||
				regions.size() != other.regions.size();
		}
	};

private:
	MapGenConf m_MapGenConf;

	float** m_NoiseMap;
	glm::vec4* m_ColorMap;
	Texture* m_Texture;
	MeshData* m_MeshData;

};
