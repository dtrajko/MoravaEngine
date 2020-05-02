#pragma once


#include "Texture.h"


class TextureCubemapLite : public Texture
{
public:
	TextureCubemapLite();
	TextureCubemapLite(unsigned int width, unsigned int height);
	virtual void Bind(unsigned int textureUnit) override;
	~TextureCubemapLite();

};
