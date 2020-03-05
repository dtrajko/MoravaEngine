#pragma once

#include <GL/glew.h>


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc);
	bool LoadTexture(bool flipVert = false);
	unsigned int GetID() const { return textureID; };
	void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void ClearTexture();
	int getRed(int x, int z);
	int getGreen(int x, int z);
	int getBlue(int x, int z);
	int getAlpha(int x, int z);
	~Texture();

	unsigned int textureID;
	int m_Width;
	int m_Height;
	int m_BitDepth;
	unsigned char* m_Buffer;
	const char* fileLocation;

};
