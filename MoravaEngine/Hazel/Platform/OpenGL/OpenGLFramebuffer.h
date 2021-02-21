#pragma once

#include "../../Renderer/HazelFramebuffer.h"


namespace Hazel {

	class OpenGLFramebuffer : public HazelFramebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void BindTexture(uint32_t slot = 0) const override;

		virtual RendererID GetRendererID() const { return m_RendererID; }
		virtual RendererID GetColorAttachmentRendererID() const { return m_ColorAttachment; }
		virtual RendererID GetDepthAttachmentRendererID() const { return m_DepthAttachment; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_Specification; }
	private:
		FramebufferSpecification m_Specification;
		RendererID m_RendererID = 0;
		RendererID m_ColorAttachment = 0, m_DepthAttachment = 0;
	};

}
