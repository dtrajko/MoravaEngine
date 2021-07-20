#pragma once

#include "Platform/OpenGL/OpenGLMoravaTexture.h"

#include <string>
#include <vector>


class TextureCubemapFaces : public OpenGLMoravaTexture
{
public:
	TextureCubemapFaces();
	TextureCubemapFaces(std::vector<std::string> faces);
	virtual ~TextureCubemapFaces();

	virtual std::pair<uint32_t, uint32_t> GetMipSize(uint32_t mip) const override;

	virtual void Bind(uint32_t textureSlot = 0) const override;

};
