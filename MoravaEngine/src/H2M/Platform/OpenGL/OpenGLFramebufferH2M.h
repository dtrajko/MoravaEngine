#define _CRT_SECURE_NO_WARNINGS

#pragma once


#include "H2M/Renderer/FramebufferH2M.h"


namespace H2M
{

	class OpenGLFramebufferH2M : public FramebufferH2M
	{
	public:
		OpenGLFramebufferH2M(const FramebufferSpecificationH2M& spec);
		virtual ~OpenGLFramebufferH2M();

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) override {}

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual RendererID_H2M GetRendererID() const { return m_RendererID; }

		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { H2M_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size()); return m_ColorAttachments[attachmentIndex]; }
		virtual RefH2M<Image2D_H2M> GetDepthImage() const override { return m_DepthAttachment; }

		virtual RendererID_H2M GetColorAttachmentRendererID(int index = 0) const { return m_ColorAttachments[index]; }
		virtual RendererID_H2M GetDepthAttachmentRendererID() const { return m_DepthAttachment; }

		virtual const FramebufferSpecificationH2M& GetSpecification() const override { return m_Specification; }

	private:
		FramebufferSpecificationH2M m_Specification;
		RendererID_H2M m_RendererID = 0;

		std::vector<RefH2M<Image2D_H2M>> m_ColorAttachments;
		RefH2M<Image2D_H2M> m_DepthAttachment;

		std::vector<ImageFormatH2M> m_ColorAttachmentFormats;
		ImageFormatH2M m_DepthAttachmentFormat = ImageFormatH2M::None;

		uint32_t m_Width = 0, m_Height = 0;

	};

}
