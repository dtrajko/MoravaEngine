#pragma once

#include <GL/glew.h>


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc);
	bool LoadTexture(bool flipVert = false);
	void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void ClearTexture();
	~Texture();

private:
	GLuint textureID;
	int width;
	int height;
	int bitDepth;

	const char* fileLocation;

};
