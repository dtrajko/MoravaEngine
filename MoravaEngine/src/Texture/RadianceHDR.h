#pragma once

#if defined(GRAPHICS_API_OPENGL)
#include "Platform/OpenGL/OpenGLMoravaTexture.h"
#endif

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
