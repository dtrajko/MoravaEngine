#pragma once

#include "Framebuffer/MoravaFramebuffer.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/HazelFramebuffer.h"

#include "Framebuffer/FramebufferTexture.h"
#include "Framebuffer/Renderbuffer.h"

#include <vector>


class OpenGLMoravaFramebuffer : public MoravaFramebuffer
{
public:
	OpenGLMoravaFramebuffer();
	OpenGLMoravaFramebuffer(unsigned int width, unsigned int height);
	OpenGLMoravaFramebuffer(FramebufferSpecification spec);
	~OpenGLMoravaFramebuffer();

	// virtual/abstract methods from HazelFramebuffer
	virtual void Unbind() const override;
	virtual void Bind() const override;
	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate) override;
	virtual void AddResizeCallback(const std::function<void(H2M::RefH2M<H2M::FramebufferH2M>)>& func) override {};
	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;
	virtual H2M::RendererID_H2M GetRendererID() const override;
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override;
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetDepthImage() const override;
	virtual const H2M::FramebufferSpecificationH2M& GetSpecification() const override;
	// virtual H2M::RendererID_H2M GetColorAttachmentRendererID() const override;
	// virtual H2M::RendererID_H2M GetDepthAttachmentRendererID() const override;

	// virtual/abstract methods from MoravaFramebuffer
	virtual void Generate(unsigned int width, unsigned int height) override; // Invalidate() in Hazel
	virtual void AddColorAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) override;
	virtual void AddDepthAttachmentSpecification(unsigned int width, unsigned int height, AttachmentType attachmentType, AttachmentFormat attachmentFormat) override;
	virtual void AddColorAttachment(FramebufferSpecification specs) override; // the generic one based on FramebufferSpecification
	virtual void AddDepthAttachment(FramebufferSpecification specs) override; // the generic one based on FramebufferSpecification
	virtual FramebufferTexture* GetTextureAttachmentColor(unsigned int orderID = 0) override;
	virtual H2M::RefH2M<Attachment> GetAttachmentDepth() override;
	virtual H2M::RefH2M<Attachment> GetAttachmentStencil() override;
	virtual H2M::RefH2M<Attachment> GetAttachmentDepthAndStencil() override;
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

	static H2M::RefH2M<MoravaFramebuffer> Create(const FramebufferSpecification& spec);

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
	H2M::RefH2M<Attachment> m_AttachmentDepth;
	H2M::RefH2M<Attachment> m_AttachmentStencil;
	H2M::RefH2M<Attachment> m_AttachmentDepthAndStencil;

	// Hazel/Platform/OpenGL/OpenGLFramebuffer
	bool m_Multisample;

	H2M::FramebufferSpecificationH2M m_HazelFramebufferSpecs; // not in use, only for compatibility with H2M::FramebufferH2M

};
