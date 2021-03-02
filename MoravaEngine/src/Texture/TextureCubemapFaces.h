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

	virtual void Bind(unsigned int textureSlot = 0) override;

};
