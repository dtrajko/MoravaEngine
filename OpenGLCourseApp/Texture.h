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
	~Texture();

private:
	unsigned int textureID;
	int width;
	int height;
	int bitDepth;

	const char* fileLocation;

};
