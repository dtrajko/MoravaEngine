#pragma once

#include <glm/glm.hpp>

#include <map>


class TerrainChunk
{
public:
	TerrainChunk();
	~TerrainChunk();
};

class EndlessTerrain
{
public:
	// properties
	const float maxViewDist = 300.0f;
	glm::mat4 viewer;
	static glm::vec2 viewerPosition;
	int chunkSize;
	int chunksVisibleInViewDst;
	std::map<glm::vec2*, TerrainChunk*>* terrainChunkDictionary;

	// methods
	EndlessTerrain();
	~EndlessTerrain();

	void Start(int mapChunkSize);
	void UpdateVisibleChunks();
};
