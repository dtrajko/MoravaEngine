#pragma once

#include "Framebuffer/Attachment.h"

#include "GL/glew.h"


class Renderbuffer : public Attachment
{
public:
	Renderbuffer();
	Renderbuffer(unsigned int width, unsigned int height, AttachmentFormat attachmentType, unsigned int orderID);
	virtual void Bind(unsigned int slot = 0) override;
	virtual void Unbind() override;
	virtual ~Renderbuffer() override;

};
