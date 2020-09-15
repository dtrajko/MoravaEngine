#pragma once

#include "Attachment.h"

#include <string>


class FramebufferTextureCubemap : public Attachment
{

public:
	FramebufferTextureCubemap();
	FramebufferTextureCubemap(unsigned int width, unsigned int height, AttachmentFormat attachmentType, unsigned int orderID);
	virtual ~FramebufferTextureCubemap() override;

	virtual void Bind(unsigned int slot = 0) override;
	virtual void Unbind() override;

private:
	int m_Level;
	int m_Border;

};
