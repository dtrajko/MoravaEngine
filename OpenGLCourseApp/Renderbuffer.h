#pragma once

#include "GL/glew.h"


class Renderbuffer
{
public:
	Renderbuffer();
	Renderbuffer(unsigned int width, unsigned int height, GLenum internalFormat);
	inline unsigned int GetID() const { return bufferID; };
	~Renderbuffer();

private:
	unsigned int bufferID;
	unsigned int m_Width;
	unsigned int m_Height;

};
