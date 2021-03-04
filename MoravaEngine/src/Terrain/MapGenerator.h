#pragma once

#include "Mesh/MeshUnity.h"
#include "Terrain/MeshGenerator.h"
#include "Texture/Texture.h"

#include <vector>


/**
 * Based on Procedural Landmass Generation series by Sebastian Lague
 */
class MapGenerator
{
public:
	enum class DrawMode {
		HeightMap,
		ColorMap,
		Mesh,
	};

	struct MapGenConf;

public:
	MapGenerator();
	MapGenerator(const char* heightMapFilePath, const char* colorMapFilePath);
	~MapGenerator();

	void Generate(MapGenerator::MapGenConf mapGenConf, float heightMapMultiplier, bool isRequiredMapRebuild, float seaLevel, int levelOfDetail);

	inline MeshUnity* GetMesh() { return m_Mesh; };
	glm::vec4 GetRegionColor(float height);

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
		int mapChunkSize;
		// int mapWidth;
		// int mapHeight;
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
				mapChunkSize  != other.mapChunkSize  ||
				// mapWidth      != other.mapWidth      ||
				// mapHeight     != other.mapHeight     ||
				noiseScale    != other.noiseScale    ||
				octaves       != other.octaves       ||
				persistance   != other.persistance   ||
				lacunarity    != other.lacunarity    ||
				seed          != other.seed          ||
				offset.x      != other.offset.x      ||
				offset.y      != other.offset.y      ||
				autoUpdate != other.autoUpdate       ||
				regions.size() != other.regions.size();
		}
	};

public:
	float** m_NoiseMap;
	glm::vec4* m_ColorMap;
	MapGenConf m_MapGenConf;

private:
	MapGenConf m_MapGenConfPrev;

	Texture* m_TextureColorMap;
	Texture* m_TextureHeightMap;
	MeshUnity* m_Mesh;

	float m_HeightMapMultiplier;
	bool m_IsRequiredMapRebuild;
	float m_SeaLevel;

	int m_LevelOfDetail;

};
