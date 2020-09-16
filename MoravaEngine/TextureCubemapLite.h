#pragma once


#include "Texture.h"


class TextureCubemapLite : public Texture
{
public:
	TextureCubemapLite();
	TextureCubemapLite(Texture::Specification spec); // constructor for fully customizable texture cubemap
	TextureCubemapLite(unsigned int width, unsigned int height);
	~TextureCubemapLite();

	virtual void OpenGLCreate() override;
	virtual void Bind(unsigned int textureUnit) override;

private:
	int m_Level;

};
