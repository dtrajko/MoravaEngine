#include "EndlessTerrain.h"



TerrainChunk::TerrainChunk()
{
}

TerrainChunk::~TerrainChunk()
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
			glm::vec2* viewedChunkCoord = new glm::vec2(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);
			
			if (terrainChunkDictionary->find(viewedChunkCoord) != terrainChunkDictionary->end()) {
			
			}
			else {
				// terrainChunkDictionary->insert(std::make_pair(viewedChunkCoord, new TerrainChunk()));
			}
		}
	}
}
