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

	virtual void Bind(uint32_t textureSlot = 0) const override;

};
