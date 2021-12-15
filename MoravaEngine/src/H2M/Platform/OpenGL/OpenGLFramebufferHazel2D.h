/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/FramebufferH2M.h"

namespace H2M
{

	class OpenGLFramebufferHazel2D : public FramebufferH2M
	{
	public:
		OpenGLFramebufferHazel2D(const FramebufferSpecificationH2M& spec);
		virtual ~OpenGLFramebufferHazel2D();

		void Invalidate();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

		virtual RendererID_H2M GetColorAttachmentRendererID(uint32_t index = 0) const override { H2M_CORE_ASSERT(index < m_ColorAttachments.size()); return m_ColorAttachments[index]; }

		virtual const FramebufferSpecificationH2M& GetSpecification() const override { return m_Specification; }

		// virtual methods from H2M::FramebufferH2M
		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) override {}
		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}
		virtual uint32_t GetWidth() const override { return m_Specification.Width; }
		virtual uint32_t GetHeight() const override { return m_Specification.Height; }
		virtual RendererID_H2M GetRendererID() const override { return m_RendererID; }
		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { Log::GetLogger()->error("Method not yet implemented!");  return RefH2M<Image2D_H2M>(); }
		virtual RefH2M<Image2D_H2M> GetDepthImage() const override { Log::GetLogger()->error("Method not yet implemented!");  return RefH2M<Image2D_H2M>(); }

	private:
		uint32_t m_RendererID = 0;
		FramebufferSpecificationH2M m_Specification;

		std::vector<FramebufferTextureSpecificationH2M> m_ColorAttachmentSpecifications;
		FramebufferTextureSpecificationH2M m_DepthAttachmentSpecification = ImageFormatH2M::None;

		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};

}
