#include "Terrain/EndlessTerrain.h"

#include "Mesh/Quad.h"


TerrainChunk::TerrainChunk()
{
}

TerrainChunk::TerrainChunk(glm::vec2 coord, int size)
{
	position = coord * glm::vec2((float)size);
	glm::vec3 positionV3 = glm::vec3(position.x, 0, position.y);

	m_Mesh = new Quad();
	m_Mesh->m_Transform.m_Position = positionV3;
	m_Mesh->m_Transform.m_Scale = glm::vec3(size / 10.0f);
}

TerrainChunk::~TerrainChunk()
{
}

void TerrainChunk::Update()
{
}

glm::vec2 EndlessTerrain::viewerPosition;

EndlessTerrain::EndlessTerrain()
{
	terrainChunkDictionary = new std::map<glm::vec2*, TerrainChunk*>();
}

EndlessTerrain::~EndlessTerrain()
{
}

void EndlessTerrain::Start(int mapChunkSize)
{
	chunkSize = mapChunkSize - 1;
	chunksVisibleInViewDst = (int)(maxViewDist / chunkSize);
}

void EndlessTerrain::UpdateVisibleChunks()
{
	int currentChunkCoordX = (int)(viewerPosition.x / chunkSize);
	int currentChunkCoordY = (int)(viewerPosition.y / chunkSize);

	for (int yOffset = -chunksVisibleInViewDst; yOffset <= chunksVisibleInViewDst; yOffset++) {
		for (int xOffset = -chunksVisibleInViewDst; xOffset <= chunksVisibleInViewDst; xOffset++) {
			glm::vec2 viewedChunkCoord = glm::vec2(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);
			
			if (terrainChunkDictionary->find(&viewedChunkCoord) != terrainChunkDictionary->end()) {
			
			}
			else {
				terrainChunkDictionary->insert(std::make_pair(&viewedChunkCoord, new TerrainChunk()));
			}
		}
	}
}
