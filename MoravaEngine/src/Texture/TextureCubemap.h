#pragma once

#include "Texture/Texture.h"


class TextureCubemap : public Texture
{
public:
	TextureCubemap();
	TextureCubemap(Texture::Specification spec); // constructor for fully customizable texture cubemap
	TextureCubemap(unsigned int width, unsigned int height);
	~TextureCubemap();

	virtual void OpenGLCreate() override;
	virtual void Bind(unsigned int textureSlot) override;

private:
	int m_Level;

};
