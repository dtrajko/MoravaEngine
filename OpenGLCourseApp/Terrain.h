#pragma once

#include "Texture.h"

#include <string>


class Terrain
{

public:
	Terrain(const char* heightMapPath, float tilingFactor, const char* colorMapPath);
	inline float* GetVertices() const { return &vertices[0]; };
	inline unsigned int* GetIndices() const { return &indices[0]; };
	inline unsigned int GetVertexCount() const { return vertexCount; };
	inline unsigned int GetIndexCount() const { return indexCount; };
	inline Texture* GetColorMap() const { return m_TxColorMap; };
	~Terrain();

private:
	void GenerateTerrain();
	float GetHeight(int x, int z);

private:

	const char* m_HeightMapPath;
	Texture* m_TxHeightMap = nullptr;
	Texture* m_TxColorMap = nullptr;

	float* vertices;
	unsigned int* indices;
	unsigned int vertexCount;
	unsigned int indexCount;

	int m_MaxHeight = 30;
	int m_MaxPixelColor = 256;
	int m_TextureTileRatio = 10;
	bool m_InvertHeight = false;

	float m_TilingFactor = 1.0f;
};
