#pragma once

#include "GL/glew.h"


enum class RenderbufferFormatType
{
	Depth =              0, // GL_DEPTH_COMPONENT
	Depth_24 =           1, // GL_DEPTH_COMPONENT24
	Depth_24_Stencil_8 = 2, // GL_DEPTH24_STENCIL8
};


class Renderbuffer
{
public:
	Renderbuffer();
	Renderbuffer(unsigned int width, unsigned int height, RenderbufferFormatType formatType, unsigned int orderID);
	void Bind();
	void Unbind();
	inline unsigned int GetID() const { return m_RBO; };
	~Renderbuffer();

private:
	unsigned int m_RBO;
	unsigned int m_Width;
	unsigned int m_Height;

};
