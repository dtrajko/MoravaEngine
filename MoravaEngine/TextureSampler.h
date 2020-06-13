#pragma once

#include "Texture.h"


/**
 * This type of Texture requires m_Buffer to be available during object lifetime
 * so m_Buffer can be deallocated only in destructor instead immediately in constructor
 */
class TextureSampler : public Texture
{
public:
	TextureSampler();
    TextureSampler(const char* fileLoc);
	~TextureSampler();

};
