#pragma once

#include "Platform/OpenGL/OpenGLMoravaTexture.h"

#include <GL/glew.h>


class RadianceHDR : public OpenGLMoravaTexture
{
public:
	RadianceHDR();
	RadianceHDR(const char* fileLoc);
	bool Load();
	~RadianceHDR();

private:
	float* m_Buffer;

};
