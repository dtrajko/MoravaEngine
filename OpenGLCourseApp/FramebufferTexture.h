#pragma once

#include "Texture.h"

#include <string>


enum class FramebufferTextureType
{
	Color = 0,
	Depth = 1,
	Stencil = 2,
	DepthStencil = 3,
};


class FramebufferTexture : public Texture
{

public:
	FramebufferTexture();
	FramebufferTexture(unsigned int width, unsigned int height, FramebufferTextureType txType, unsigned int orderID);
	~FramebufferTexture();

};
