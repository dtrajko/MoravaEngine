#pragma once

#include "FramebufferTexture.h"
#include "Renderbuffer.h"
#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelFramebuffer.h"
#include "Hazel/Renderer/RendererAPI.h"

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


class Framebuffer : public Hazel::HazelFramebuffer
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

	void CreateAttachment(FramebufferSpecification specs); // the generic one based on FramebufferSpecification 
	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);

	FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0);
	Attachment* GetAttachmentDepth();
	Attachment* GetAttachmentStencil();
	Attachment* GetAttachmentDepthAndStencil();

	FramebufferSpecification& GetSpecification() { return m_FramebufferSpecs; };

	static Hazel::Ref<Framebuffer> Create(const FramebufferSpecification& spec);

	inline const uint32_t GetWidth() const { return m_FramebufferSpecs.Width; };
	inline const uint32_t GetHeight() const { return m_FramebufferSpecs.Height; };
	inline const uint32_t GetID() const { return m_FBO; };

	void Clear();

	void Release();
	void Generate(unsigned int width, unsigned int height); // Invalidate() in Hazel
	void Resize(uint32_t width, uint32_t height);

	// HazelFramebuffer abstract methods
	virtual void Bind() const override;
	virtual void Unbind() const override;
	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate) override;
	virtual void BindTexture(uint32_t slot = 0) const override;
	virtual Hazel::RendererID GetRendererID() const override;
	virtual Hazel::RendererID GetColorAttachmentRendererID() const override;
	virtual Hazel::RendererID GetDepthAttachmentRendererID() const override;
	virtual const Hazel::HazelFramebufferSpecification& GetSpecification() const override;

private:
	unsigned int m_FBO;
	FramebufferSpecification m_FramebufferSpecs;

	std::vector<FramebufferSpecification> m_AttachmentSpecs;
	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	Attachment* m_AttachmentDepth;
	Attachment* m_AttachmentStencil;
	Attachment* m_AttachmentDepthAndStencil;

	// Hazel/Platform/OpenGL/OpenGLFramebuffer
	bool m_Multisample;

};
