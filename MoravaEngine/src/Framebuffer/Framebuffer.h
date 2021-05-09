#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelFramebuffer.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Framebuffer/FramebufferTexture.h"
#include "Framebuffer/Renderbuffer.h"

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

	virtual void Unbind() const override;
	virtual void Bind() const override;

	void Bind(unsigned int width, unsigned int height);
	void Unbind(unsigned int width, unsigned int height);
	bool CheckStatus();

	void AddColorAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat);
	void AddDepthAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat);

	void AddColorAttachment(FramebufferSpecification specs); // the generic one based on FramebufferSpecification 
	void AddDepthAttachment(FramebufferSpecification specs); // the generic one based on FramebufferSpecification 

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);

	FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0);
	Hazel::Ref<Attachment> GetAttachmentDepth();
	Hazel::Ref<Attachment> GetAttachmentStencil();
	Hazel::Ref<Attachment> GetAttachmentDepthAndStencil();

	FramebufferSpecification& GetSpecification() { return m_FramebufferSpecs; };

	static Hazel::Ref<Framebuffer> Create(const FramebufferSpecification& spec);

	inline uint32_t GetWidth() const { return m_FramebufferSpecs.Width; };
	inline uint32_t GetHeight() const { return m_FramebufferSpecs.Height; };
	inline const uint32_t GetID() const { return m_FBO; };

	void Clear();

	void Release();
	void Generate(unsigned int width, unsigned int height); // Invalidate() in Hazel
	void Resize(uint32_t width, uint32_t height);

	// HazelFramebuffer abstract methods
	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate) override;
	virtual void AddResizeCallback(const std::function<void(Hazel::Ref<Hazel::HazelFramebuffer>)>& func) override;
	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;
	virtual Hazel::RendererID GetRendererID() const override;

	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const override;
	virtual Hazel::Ref<Hazel::HazelImage2D> GetDepthImage() const override;

	// virtual Hazel::RendererID GetColorAttachmentRendererID() const override;
	// virtual Hazel::RendererID GetDepthAttachmentRendererID() const override;

	virtual const Hazel::HazelFramebufferSpecification& GetSpecification() const override;

private:
	unsigned int m_FBO;
	FramebufferSpecification m_FramebufferSpecs;

	std::vector<FramebufferSpecification> m_ColorAttachmentSpecs;
	std::vector<FramebufferSpecification> m_RenderbufferAttachmentSpec;

	std::vector<FramebufferTexture*> m_TextureAttachmentsColor;
	Hazel::Ref<Attachment> m_AttachmentDepth;
	Hazel::Ref<Attachment> m_AttachmentStencil;
	Hazel::Ref<Attachment> m_AttachmentDepthAndStencil;

	// Hazel/Platform/OpenGL/OpenGLFramebuffer
	bool m_Multisample;

	Hazel::HazelFramebufferSpecification m_HazelFramebufferSpecs; // not in use, only for compatibility with Hazel::HazelFramebuffer

};
