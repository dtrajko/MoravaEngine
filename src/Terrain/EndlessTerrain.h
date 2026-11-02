#pragma once

#include "Mesh/Mesh.h"

#include <glm/glm.hpp>

#include <map>


class TerrainChunk
{
public:
	TerrainChunk();
	TerrainChunk(glm::vec2 coord, int size);
	~TerrainChunk();

	void Update();

public:
	glm::vec2 position;
	Mesh* m_Mesh;

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
