#pragma once

#include "DX11.h"

#include "DX11Texture2D.h"

#include "H2M/Renderer/FramebufferH2M.h"


class DX11Framebuffer : public H2M::FramebufferH2M
{
public:
	DX11Framebuffer(const H2M::FramebufferSpecificationH2M& spec);
	virtual ~DX11Framebuffer();

	virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;
	virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

	virtual void AddResizeCallback(const std::function<void(H2M::RefH2M<H2M::FramebufferH2M>)>& func) override;

	virtual void Bind() const override {}
	virtual void Unbind() const override {}

	virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

	virtual uint32_t GetWidth() const override { return m_Width; }
	virtual uint32_t GetHeight() const override { return m_Height; }

	virtual H2M::RendererID_H2M GetRendererID() const { return m_RendererID; }

	virtual H2M::RefH2M<H2M::Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { H2M_CORE_ASSERT(attachmentIndex < m_Attachments.size()); return m_Attachments[attachmentIndex]; }
	virtual H2M::RefH2M<H2M::Image2D_H2M> GetDepthImage() const override { return H2M::RefH2M<H2M::Image2D_H2M>(); /* m_DepthAttachment; */ }

	virtual H2M::RendererID_H2M GetColorAttachmentRendererID() const { return 0; }
	virtual H2M::RendererID_H2M GetDepthAttachmentRendererID() const { return 0; }

	size_t GetColorAttachmentCount() const { return m_Attachments.size(); }
	virtual const H2M::FramebufferSpecificationH2M& GetSpecification() const override { return m_Specification; }

private:
	H2M::FramebufferSpecificationH2M m_Specification;
	H2M::RendererID_H2M m_RendererID = 0;
	uint32_t m_Width = 0, m_Height = 0;

	std::vector<H2M::RefH2M<H2M::Image2D_H2M>> m_Attachments;

	std::vector<std::function<void(H2M::RefH2M<FramebufferH2M>)>> m_ResizeCallbacks;

	// DirectX 11 framebuffer resources (added here as a reminder, not used yet)
	H2M::RefH2M<DX11Texture2D> m_RenderTarget;
	H2M::RefH2M<DX11Texture2D> m_DepthStencil;

};
