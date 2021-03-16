#pragma once

#include "Framebuffer/ShadowMap.h"


class OmniShadowMap : public ShadowMap
{
public:
	OmniShadowMap();
	virtual ~OmniShadowMap();

	virtual bool Init(GLuint width, GLuint height) override;
	virtual void ReadTexture(unsigned int textureUnit) override;
};
