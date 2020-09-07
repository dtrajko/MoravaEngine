#pragma once


#include "Texture.h"


class TextureCubemapLite : public Texture
{
public:
	TextureCubemapLite();
	TextureCubemapLite(unsigned int width, unsigned int height);
	~TextureCubemapLite();
	
	virtual void Bind(unsigned int textureUnit) override;

};
