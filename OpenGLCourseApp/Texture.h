#pragma once

#include <GL/glew.h>

#include "stb_image.h"


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc);

	bool LoadTexture(bool flipVert = false);
	void UseTexture(unsigned int textureUnit = 0);
	void ClearTexture();

	~Texture();

private:
	GLuint textureID;
	int width;
	int height;
	int bitDepth;

	const char* fileLocation;

};
