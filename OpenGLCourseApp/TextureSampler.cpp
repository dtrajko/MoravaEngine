#include "TextureSampler.h"


TextureSampler::TextureSampler() : Texture()
{
	m_IsSampler = true;
}

TextureSampler::TextureSampler(const char* fileLoc) : Texture(fileLoc, false, true)
{
}

TextureSampler::~TextureSampler()
{
}
