#pragma once

#include "Platform/OpenGL/OpenGLMoravaTexture.h"


/**
 * This type of Texture requires m_Buffer to be available during object lifetime
 * so m_Buffer can be deallocated only in destructor instead immediately in constructor
 */
class TextureSampler : public OpenGLMoravaTexture
{
public:
	TextureSampler();
    TextureSampler(const char* fileLoc);
	~TextureSampler();

};
