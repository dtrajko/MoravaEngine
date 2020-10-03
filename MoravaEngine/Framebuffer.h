#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"

#include <vector>


struct FramebufferSpecification
{
	uint32_t Width = 1280;
	uint32_t Height = 720;
	glm::vec4 ClearColor;

	AttachmentType attachmentType;
	AttachmentFormat attachmentFormat;

	uint32_t Samples = 1; // multisampling

	// SwapChainTarget = screen buffer (i.e. no framebuffer)
	bool SwapChainTarget = false;
};

class Framebuffer
{
public:
	Framebuffer();
	Framebuffer(unsigned int width, unsigned int height);
	Framebuffer(FramebufferSpecification spec);
	~Framebuffer();

	void Bind();
	void Unbind();

	void Bind(unsigned int width, unsigned int height);
	void Unbind(unsigned int width, unsigned int height);
	bool CheckStatus();

	void AddAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat);

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);
	void CreateAttachmentRGBA16F(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::RGBA16F);
	void CreateAttachmentRGBA8(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::RGBA8);

	FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0);
	Attachment* GetAttachmentDepth();
	Attachment* GetAttachmentStencil();
	Attachment* GetAttachmentDepthAndStencil();

	std::vector<FramebufferSpecification>& GetSpecification() { return m_AttachmentSpecs; };

	inline const uint32_t GetWidth() const { return m_Width; };
	inline const uint32_t GetHeight() const { return m_Height; };

	void Clear();

	void Release();
	void Generate(unsigned int width, unsigned int height); // Invalidate() in Hazel
	void Resize(uint32_t width, uint32_t height);

private:
	uint32_t m_Width;
	uint32_t m_Height;

	std::vector<FramebufferSpecification> m_AttachmentSpecs;

	unsigned int m_FBO;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	Attachment* m_AttachmentDepth;
	Attachment* m_AttachmentStencil;
	Attachment* m_AttachmentDepthAndStencil;

};
