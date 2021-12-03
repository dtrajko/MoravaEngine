#pragma once

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"


namespace H2M {

	class VulkanFramebufferH2M : public FramebufferH2M
	{
	public:
		VulkanFramebufferH2M(const HazelFramebufferSpecification& spec);

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		virtual void AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func) override;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual RendererID_H2M GetRendererID() const { return m_RendererID; }

		virtual RefH2M<Image2D_H2M> GetImage(uint32_t attachmentIndex = 0) const override { H2M_CORE_ASSERT(attachmentIndex < m_Attachments.size()); return m_Attachments[attachmentIndex]; }
		virtual RefH2M<Image2D_H2M> GetDepthImage() const override { return RefH2M<Image2D_H2M>(); /* m_DepthAttachment; */ }

		virtual RendererID_H2M GetColorAttachmentRendererID() const { return 0; }
		virtual RendererID_H2M GetDepthAttachmentRendererID() const { return 0; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }

		size_t GetColorAttachmentCount() const { return m_Attachments.size(); }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return m_ClearValues; }
		virtual const HazelFramebufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		HazelFramebufferSpecification m_Specification;
		RendererID_H2M m_RendererID = 0;
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
