#pragma once


class ParticleTexture
{
public:
	ParticleTexture();
	ParticleTexture(int textureID, int numberOfRows);
	inline int GetTextureID() const { return m_TextureID; };
	inline int GetNumberOfRows() const { return m_NumberOfRows; };
	void Bind(unsigned int textureUnit = 0);
	~ParticleTexture();

private:
	int m_TextureID;
	int m_NumberOfRows;

};
