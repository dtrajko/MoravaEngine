#pragma once

#include "FramebufferTexture.h"

#include <map>
#include <string>


/**
GL_COLOR_ATTACHMENT0
GL_DEPTH_ATTACHMENT
GL_STENCIL_ATTACHMENT
GL_DEPTH_STENCIL_ATTACHMENT

Each attachment is either a Texture or Renderbuffer Object
*/

class Framebuffer
{

public:
	Framebuffer();
	Framebuffer(int width, int height);
	bool Init();
	void Write();
	void Read(unsigned int textureUnit);
	~Framebuffer();

private:
	unsigned int fboID;
	unsigned int textureID;
	int m_Width;
	int m_Height;

	std::map<std::string, FramebufferTexture*> attachments;

};
