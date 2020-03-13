#pragma once

#include <GL/glew.h>


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc);
	bool Load(bool flipVert = false);
	inline unsigned int GetID() const { return m_TextureID; };
	void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void Clear();
	inline unsigned int GetWidth() const { return m_Width; };
	inline unsigned int GetHeight() const { return m_Height; };
	int getRed(int x, int z);
	int getGreen(int x, int z);
	int getBlue(int x, int z);
	int getAlpha(int x, int z);
	~Texture();

protected:
	unsigned int m_Width;
	unsigned int m_Height;
	unsigned int m_TextureID;
	const char* m_FileLocation;
	int m_BitDepth;
	unsigned char* m_Buffer;

};
