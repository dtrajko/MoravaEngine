#include "Texture.h"

#include "CommonValues.h"



Texture::Texture()
{
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	fileLocation = "";
}

Texture::Texture(const char* fileLoc)
	: Texture()
{
	fileLocation = fileLoc;
}

bool Texture::LoadTexture(bool flipVert)
{
	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);
	unsigned char* texData = stbi_load(fileLocation, &width, &height, &bitDepth, 0);
	if (!texData)
	{
		printf("Failed to find: '%s'\n", fileLocation);
		return false;
	}

	GLenum internalFormat = 0;
	GLenum dataFormat = 0;
	if (bitDepth == 4)
	{
		internalFormat = GL_RGBA8;
		dataFormat = GL_RGBA;
	}
	else if (bitDepth == 3)
	{
		internalFormat = GL_RGB8;
		dataFormat = GL_RGB;
	}

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, texData);
	glGenerateMipmap(GL_TEXTURE_2D);

	printf("Loading texture '%s'\n", fileLocation);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(texData);

	return true;
}

void Texture::Bind(unsigned int textureUnit)
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureID);
	// printf("Bind texture ID=%d to slot=%d\n", textureID, textureUnit);
}

void Texture::Unbind()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::ClearTexture()
{
	glDeleteTextures(1, &textureID);
	textureID = 0;
	width = 0;
	height = 0;
	bitDepth = 0;
	fileLocation = "";
}

Texture::~Texture()
{
	ClearTexture();
}
