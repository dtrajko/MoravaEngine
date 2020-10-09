#pragma once

#include "Texture.h"

#include <vector>
#include <string>


class TextureCubemapFaces : public Texture
{
public:
	TextureCubemapFaces();
	TextureCubemapFaces(std::vector<std::string> faces);
	virtual ~TextureCubemapFaces();

	virtual void Bind(unsigned int textureSlot = 0) override;

};
