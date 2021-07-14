#pragma once

#include "Framebuffer/MoravaFramebuffer.h"

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/HazelFramebuffer.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Framebuffer/FramebufferTexture.h"
#include "Framebuffer/Renderbuffer.h"

#include <vector>


class DX11MoravaFramebuffer : public MoravaFramebuffer
{
public:
	DX11MoravaFramebuffer();
	DX11MoravaFramebuffer(unsigned int width, unsigned int height);
	DX11MoravaFramebuffer(FramebufferSpecification spec);
	~DX11MoravaFramebuffer();

	// virtual/abstract methods from HazelFramebuffer
	virtual void Unbind() const override;
	virtual void Bind() const override;
	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate) override;
	virtual void AddResizeCallback(const std::function<void(Hazel::Ref<Hazel::HazelFramebuffer>)>& func) override {};
	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;
	virtual Hazel::RendererID GetRendererID() const override;
	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const override;
	virtual Hazel::Ref<Hazel::HazelImage2D> GetDepthImage() const override;
	virtual const Hazel::HazelFramebufferSpecification& GetSpecification() const override;
	// virtual Hazel::RendererID GetColorAttachmentRendererID() const override;
	// virtual Hazel::RendererID GetDepthAttachmentRendererID() const override;

	// virtual/abstract methods from MoravaFramebuffer
	virtual void Generate(unsigned int width, unsigned int height) override; // Invalidate() in Hazel
	virtual void AddColorAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) override;
	virtual void AddDepthAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) override;
	virtual void AddColorAttachment(FramebufferSpecification specs) override; // the generic one based on FramebufferSpecification
	virtual void AddDepthAttachment(FramebufferSpecification specs) override; // the generic one based on FramebufferSpecification
	virtual FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0) override;
	virtual Hazel::Ref<Attachment> GetAttachmentDepth() override;
	virtual Hazel::Ref<Attachment> GetAttachmentStencil() override;
	virtual Hazel::Ref<Attachment> GetAttachmentDepthAndStencil() override;
	virtual void Bind(unsigned int width, unsigned int height) override;
	virtual void Unbind(unsigned int width, unsigned int height) override;
	virtual bool CheckStatus() override;
	virtual void Clear() override;

	void CreateTextureAttachmentColor(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentFormat attachmentFormat = AttachmentFormat::Color);
	void CreateAttachmentDepth(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth);
	void CreateAttachmentStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Stencil);
	void CreateAttachmentDepthAndStencil(unsigned int width, unsigned int height, bool isMultisample,
		AttachmentType attachmentType, AttachmentFormat attachmentFormat = AttachmentFormat::Depth_24_Stencil_8);

	FramebufferSpecification& GetSpecification() { return m_FramebufferSpecs; };

	static Hazel::Ref<MoravaFramebuffer> Create(const FramebufferSpecification& spec);

	inline uint32_t GetWidth() const { return m_FramebufferSpecs.Width; };
	inline uint32_t GetHeight() const { return m_FramebufferSpecs.Height; };
	inline const uint32_t GetID() const { return m_FBO; };

	void Release();
	void Resize(uint32_t width, uint32_t height);

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
