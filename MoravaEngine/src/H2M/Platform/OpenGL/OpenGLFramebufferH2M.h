/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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

		void Invalidate();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

		virtual RendererID_H2M GetColorAttachmentRendererID(uint32_t index = 0) const override { H2M_CORE_ASSERT(index < m_ColorAttachmentIDs.size()); return m_ColorAttachmentIDs[index]; }
		virtual RendererID_H2M GetDepthAttachmentRendererID() const { return m_DepthAttachmentID; }

		virtual const FramebufferSpecificationH2M& GetSpecification() const override { return m_Specification; }

		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) override {}

		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }

		virtual RendererID_H2M GetRendererID() const override { return m_RendererID; }

		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { H2M_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size()); return m_ColorAttachments[attachmentIndex]; }
		virtual RefH2M<Image2D_H2M> GetDepthImage() const override { return m_DepthAttachment; }

	private:
		FramebufferSpecificationH2M m_Specification;
		RendererID_H2M m_RendererID = 0;

		std::vector<RefH2M<Image2D_H2M>> m_ColorAttachments;
		RefH2M<Image2D_H2M> m_DepthAttachment;

		std::vector<RendererID_H2M> m_ColorAttachmentIDs;
		RendererID_H2M m_DepthAttachmentID = 0;

		// std::vector<ImageFormatH2M> m_ColorAttachmentFormats;
		// ImageFormatH2M m_DepthAttachmentFormat = ImageFormatH2M::None;

		std::vector<FramebufferTextureSpecificationH2M> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecificationH2M m_DepthAttachmentSpecification = ImageFormatH2M::None;

	};

}
