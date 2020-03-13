#pragma once

#include <GL/glew.h>

#include "Texture.h"


class TextureCubemap : public Texture
{
public:
	TextureCubemap();
	TextureCubemap(unsigned int width, unsigned int height);
	~TextureCubemap();

};
