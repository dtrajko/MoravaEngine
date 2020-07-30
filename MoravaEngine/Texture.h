#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <string>

#include "CommonValues.h"


class Texture
{
public:
	Texture();
	Texture(const char* fileLoc, bool flipVert = false);
	Texture(const char* fileLoc, bool flipVert, GLenum filter);
	Texture(const char* fileLoc, bool flipVert, bool isSampler);
	Texture(const char* fileLoc, unsigned int width, unsigned int height, bool isSampler, GLenum filter);
	virtual bool Load(bool flipVert = false);
	virtual void OpenGLCreate();
	virtual void Save();
	inline unsigned int GetID() const { return m_TextureID; };
	virtual void Bind(unsigned int textureUnit = 0);
	void Unbind();
	void Clear();
	inline unsigned int GetWidth() const { return m_Width; };
	inline unsigned int GetMaxY() const { return m_Height; };

	// Getters
	int GetRed(int x, int z);
	int GetGreen(int x, int z);
	int GetBlue(int x, int z);
	int GetAlpha(int x, int z);

	// Setters
	void SetPixel(int x, int z, glm::ivec4 pixel);
	void SetRed(int x, int z, int value);
	void SetGreen(int x, int z, int value);
	void SetBlue(int x, int z, int value);
	void SetAlpha(int x, int z, int value);

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
