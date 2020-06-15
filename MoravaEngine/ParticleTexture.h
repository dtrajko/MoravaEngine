#pragma once


class ParticleTexture
{
public:
	ParticleTexture();
	ParticleTexture(int textureID, int numberOfRows);
	inline const int GetTextureID() const { return m_TextureID; };
	inline const int GetNumberOfRows() const { return m_NumberOfRows; };
	void Bind(unsigned int textureUnit = 0);
	~ParticleTexture();

private:
	int m_TextureID;
	int m_NumberOfRows;

};
