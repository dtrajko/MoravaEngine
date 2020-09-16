#pragma once

#include "FramebufferTexture.h"
#include "Texture.h"

#include <string>


class FramebufferTextureCubemap : public FramebufferTexture
{

public:
	FramebufferTextureCubemap();
	FramebufferTextureCubemap(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat, unsigned int orderID);
	FramebufferTextureCubemap(Texture::Specification spec, unsigned int orderID);
	virtual ~FramebufferTextureCubemap() override;

	virtual void InitSpecification() override;
	virtual void OpenGLCreate() override;

	virtual void Bind(unsigned int slot = 0) override;
	virtual void Unbind() override;

};
