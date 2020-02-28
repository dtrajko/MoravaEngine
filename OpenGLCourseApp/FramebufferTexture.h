#pragma once


#include "Texture.h"

#include <string>


class FramebufferTexture : public Texture
{

public:
	FramebufferTexture();
	FramebufferTexture(int width, int height, std::string type);
	~FramebufferTexture();

};
