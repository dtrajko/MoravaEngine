#pragma once

#include "Platform/OpenGL/OpenGLMoravaTexture.h"


class TextureCubemap : public OpenGLMoravaTexture
{
public:
	TextureCubemap();
	TextureCubemap(MoravaTexture::Specification spec); // constructor for fully customizable texture cubemap
	TextureCubemap(unsigned int width, unsigned int height);
	~TextureCubemap();

	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	virtual void CreateAPISpecific() override;
	virtual void Bind(uint32_t textureSlot = 0) const override;

private:
	int m_Level;

};
