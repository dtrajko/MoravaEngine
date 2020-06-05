#pragma once

#include "Texture.h"
#include "Mesh.h"

#include <string>


class Terrain : public Mesh
{

public:
	Terrain(const char* heightMapPath, float tilingFactor, const char* colorMapPath);
	virtual void Generate(glm::vec3 scale) override;
	inline Texture* GetHeightMap() const { return m_TxHeightMap; };
	inline Texture* GetColorMap() const { return m_TxColorMap; };
	float GetHeight(int x, int z);
	~Terrain();

private:
	glm::vec3 m_ScalePrev;

	const char* m_HeightMapPath;
	Texture* m_TxHeightMap = nullptr;
	Texture* m_TxColorMap = nullptr;

	int m_MaxHeight = 30;
	int m_MaxPixelColor = 256;
	int m_TextureTileRatio = 10;
	bool m_InvertHeight = false;

	float m_TilingFactor = 1.0f;
};
