#pragma once

#include "Texture/Texture.h"

#include <string>
#include <vector>


class TextureCubemapFaces : public Texture
{
public:
	TextureCubemapFaces();
	TextureCubemapFaces(std::vector<std::string> faces);
	virtual ~TextureCubemapFaces();

	virtual void Bind(uint32_t textureSlot = 0) const override;

};
