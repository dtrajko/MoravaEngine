#pragma once

#include "Framebuffer/Attachment.h"
#include "Texture/Texture.h"

#include <string>


class FramebufferTexture : public Attachment
{

public:
	FramebufferTexture();
	FramebufferTexture(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentFormat attachmentFormat, unsigned int orderID);
	FramebufferTexture(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat, unsigned int orderID);
	FramebufferTexture(Texture::Specification spec, unsigned int orderID); // constructor for fully customizable framebuffer texture
	virtual ~FramebufferTexture() override;

	virtual void InitSpecification();
	virtual void OpenGLCreate();

	virtual void Bind(unsigned int slot = 0) override;
	virtual void Unbind() override;

	static GLenum TextureTarget(bool multisampled);

public:
	Texture::Specification m_Spec;

protected:
	int m_Level;
	int m_Attachment;

};
