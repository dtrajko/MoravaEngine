#include "Texture/TextureSampler.h"


TextureSampler::TextureSampler() : OpenGLMoravaTexture()
{
	m_Spec.IsSampler = true;
}

TextureSampler::TextureSampler(const char* fileLoc) : OpenGLMoravaTexture(fileLoc, false, true)
{
}

TextureSampler::~TextureSampler()
{
}
