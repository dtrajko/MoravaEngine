#pragma once

#include "Texture/MoravaTexture.h"

#include <glm/glm.hpp>


class SubTexture2D
{
public:
	SubTexture2D();
	SubTexture2D(H2M::RefH2M<MoravaTexture> texture, const glm::vec2& min, const glm::vec2& max);

	const H2M::RefH2M<MoravaTexture> GetTexture() const { return m_Texture; };
	const glm::vec2* GetTexCoords() const { return m_TexCoords; };

	static SubTexture2D CreateFromCoords(H2M::RefH2M<MoravaTexture> texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize);
	virtual ~SubTexture2D();

private:
	H2M::RefH2M<MoravaTexture> m_Texture;
	glm::vec2 m_TexCoords[4];

};
