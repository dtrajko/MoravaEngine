#pragma once

#include "Attachment.h"

#include <string>


class FramebufferTexture : public Attachment
{

public:
	FramebufferTexture();
	FramebufferTexture(unsigned int width, unsigned int height, AttachmentFormat attachmentType, unsigned int orderID);
	virtual void Bind(unsigned int slot = 0) override;
	virtual void Unbind() override;
	~FramebufferTexture();

};
