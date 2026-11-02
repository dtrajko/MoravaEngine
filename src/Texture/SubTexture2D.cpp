#include "Texture/SubTexture2D.h"


SubTexture2D::SubTexture2D()
{
	m_TexCoords[0] = { 0.0f, 0.0f };
	m_TexCoords[1] = { 0.0f, 0.0f };
	m_TexCoords[2] = { 0.0f, 0.0f };
	m_TexCoords[3] = { 0.0f, 0.0f };
}

SubTexture2D::SubTexture2D(H2M::RefH2M<MoravaTexture> texture, const glm::vec2& min, const glm::vec2& max)
	: m_Texture(texture)
{
	m_TexCoords[0] = { min.x, min.y };
	m_TexCoords[1] = { max.x, min.y };
	m_TexCoords[2] = { max.x, max.y };
	m_TexCoords[3] = { min.x, max.y };
}

SubTexture2D SubTexture2D::CreateFromCoords(H2M::RefH2M<MoravaTexture> texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize)
{
	glm::vec2 min = { ((coords.x + 0) * cellSize.x) / texture->GetWidth(),  ((coords.y + 0) * cellSize.y) / texture->GetHeight() }; // { 0.0f, 0.0f }
	glm::vec2 max = { ((coords.x + spriteSize.x) * cellSize.x) / texture->GetWidth(),  ((coords.y + spriteSize.y) * cellSize.y) / texture->GetHeight() }; // { 1.0f, 1.0f }
	return SubTexture2D(texture, min, max);
}

SubTexture2D::~SubTexture2D()
{
}
