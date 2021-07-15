#pragma once

#include "Framebuffer/FramebufferTexture.h"
#include "Texture/MoravaTexture.h"

#include <string>


class FramebufferTextureCubemap : public FramebufferTexture
{

public:
	FramebufferTextureCubemap();
	FramebufferTextureCubemap(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentFormat attachmentFormat, unsigned int orderID);
	FramebufferTextureCubemap(MoravaTexture::Specification spec, unsigned int orderID);
	virtual ~FramebufferTextureCubemap() override;

	virtual void InitSpecification() override;
	virtual void OpenGLCreate() override;

	virtual void Bind(unsigned int slot = 0) const override;
	virtual void Unbind() override;

};
