/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"


namespace H2M {

	class VulkanFramebufferH2M : public FramebufferH2M
	{
	public:
		VulkanFramebufferH2M(const FramebufferSpecificationH2M& spec);

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;
		void ResizeEnvMapVulkan(uint32_t width, uint32_t height, bool forceRecreate = false);
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;

		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) override;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual uint32_t GetRendererID() const { return m_RendererID; }

		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { H2M_CORE_ASSERT(attachmentIndex < m_Attachments.size()); return m_Attachments[attachmentIndex]; }
		virtual RefH2M<Image2D_H2M> GetDepthImage() const override { return RefH2M<Image2D_H2M>(); /* m_DepthAttachment; */ }

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const { return 0; }
		virtual uint32_t GetDepthAttachmentRendererID() const { return 0; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }

		size_t GetColorAttachmentCount() const { return m_Attachments.size(); }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return m_ClearValues; }
		virtual const FramebufferSpecificationH2M& GetSpecification() const override { return m_Specification; }

		void Invalidate();
		void RT_Invalidate();

		// virtual methods from OpenGLFramebufferHazel2D
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override { Log::GetLogger()->error("Method not yet implemented!"); }

	private:
		FramebufferSpecificationH2M m_Specification;
		uint32_t m_RendererID = 0;
		uint32_t m_Width = 0, m_Height = 0;

		std::vector<RefH2M<Image2D_H2M>> m_Attachments;
		std::vector<RefH2M<Image2D_H2M>> m_AttachmentImages;
		RefH2M<Image2D_H2M>m_DepthAttachmentImage;

		std::vector<VkClearValue> m_ClearValues;


		struct FrameBufferAttachment
		{
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};

		FrameBufferAttachment m_ColorAttachment, m_DepthAttachment;
		VkSampler m_ColorAttachmentSampler;
		VkRenderPass m_RenderPass = nullptr;
		VkFramebuffer m_Framebuffer = nullptr;
		VkDescriptorImageInfo m_DescriptorImageInfo;

		std::vector<std::function<void(RefH2M<FramebufferH2M>)>> m_ResizeCallbacks;

	};

}
