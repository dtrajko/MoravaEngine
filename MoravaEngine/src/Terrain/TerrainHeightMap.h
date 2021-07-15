#pragma once

#include "Terrain/TerrainBase.h"
#include "Texture/MoravaTexture.h"

#include <string>


class TerrainHeightMap : public TerrainBase
{

public:
	TerrainHeightMap(const char* heightMapPath, float tilingFactor, const char* colorMapPath);
	~TerrainHeightMap();

	virtual void Generate(glm::vec3 scale) override;

	inline Hazel::Ref<MoravaTexture> GetHeightMap() const { return m_TxHeightMap; };
	inline Hazel::Ref<MoravaTexture> GetColorMap() const { return m_TxColorMap; };
	virtual float GetMaxY(int x, int z) override;

private:
	glm::vec3 m_ScalePrev;

	const char* m_HeightMapPath;
	Hazel::Ref<MoravaTexture> m_TxHeightMap;
	Hazel::Ref<MoravaTexture> m_TxColorMap;

	int m_MaxHeight = 30;
	int m_MaxPixelColor = 256;
	int m_TextureTileRatio = 10;
	bool m_InvertHeight = false;

	float m_TilingFactor = 1.0f;
};
