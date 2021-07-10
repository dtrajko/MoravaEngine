#pragma once

#include "DX11.h"

#include "DX11Texture2D.h"

#include "Hazel/Renderer/HazelFramebuffer.h"


class DX11Framebuffer : public Hazel::HazelFramebuffer
{
public:
	DX11Framebuffer(const Hazel::HazelFramebufferSpecification& spec);
	virtual ~DX11Framebuffer();

	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

	virtual void AddResizeCallback(const std::function<void(Hazel::Ref<Hazel::HazelFramebuffer>)>& func) override;

	virtual void Bind() const override {}
	virtual void Unbind() const override {}

	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }

	virtual Hazel::RendererID GetRendererID() const { return m_RendererID; }

	virtual Hazel::Ref<Hazel::HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const override { HZ_CORE_ASSERT(attachmentIndex < m_Attachments.size()); return m_Attachments[attachmentIndex]; }
	virtual Hazel::Ref<Hazel::HazelImage2D> GetDepthImage() const override { return Hazel::Ref<Hazel::HazelImage2D>(); /* m_DepthAttachment; */ }

	virtual Hazel::RendererID GetColorAttachmentRendererID() const { return 0; }
	virtual Hazel::RendererID GetDepthAttachmentRendererID() const { return 0; }

	size_t GetColorAttachmentCount() const { return m_Attachments.size(); }
	virtual const Hazel::HazelFramebufferSpecification& GetSpecification() const override { return m_Specification; }

private:
	Hazel::HazelFramebufferSpecification m_Specification;
	Hazel::RendererID m_RendererID = 0;
	uint32_t m_Width = 0, m_Height = 0;

	std::vector<Hazel::Ref<Hazel::HazelImage2D>> m_Attachments;

	std::vector<std::function<void(Hazel::Ref<HazelFramebuffer>)>> m_ResizeCallbacks;

	// DirectX 11 framebuffer resources (added here as a reminder, not used yet)
	Hazel::Ref<DX11Texture2D> m_RenderTarget;
	Hazel::Ref<DX11Texture2D> m_DepthStencil;

};
