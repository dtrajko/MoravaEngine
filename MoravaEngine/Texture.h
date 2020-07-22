#pragma once

#include <GL/glew.h>

#include <string>

#include "CommonValues.h"


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc, bool flipVert = false);
	Texture(const char* fileLoc, bool flipVert, GLenum filter);
	Texture(const char* fileLoc, bool flipVert, bool isSampler);
	virtual bool Load(bool flipVert = false);
	inline unsigned int GetID() const { return m_TextureID; };
	virtual void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void Clear();
	inline unsigned int GetWidth() const { return m_Width; };
	inline unsigned int GetMaxY() const { return m_Height; };
	int getRed(int x, int z);
	int getGreen(int x, int z);
	int getBlue(int x, int z);
	int getAlpha(int x, int z);
	float GetFileSize(const char* filename);
	~Texture();

protected:
	unsigned int m_Width;
	unsigned int m_Height;
	unsigned int m_TextureID = -1;
	const char* m_FileLocation;
	int m_BitDepth;
	unsigned char* m_Buffer = nullptr;
	bool m_IsSampler; // m_Buffer is required during object lifetime, so it must be deallocated in destructor
	GLenum m_Filter = GL_LINEAR;

};
