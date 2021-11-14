#pragma once

#include "Hazel/Platform/Vulkan/Vulkan.h"

#include "Hazel/Renderer/HazelFramebuffer.h"


namespace Hazel {

	class VulkanFramebufferHazelLegacy : public HazelFramebuffer
	{
	public:
		VulkanFramebufferHazelLegacy(const HazelFramebufferSpecification& spec);

		virtual void Resize(uint32_t width, uint32_t height, bool forceRecreate = false) override;

		virtual void AddResizeCallback(const std::function<void(Ref<HazelFramebuffer>)>& func) override;

		virtual void Bind() const override {}
		virtual void Unbind() const override {}

		virtual void BindTexture(uint32_t attachmentIndex = 0, uint32_t slot = 0) const override {}

		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }

		virtual RendererID GetRendererID() const { return m_RendererID; }

		virtual Ref<HazelImage2D> GetImage(uint32_t attachmentIndex = 0) const override { HZ_CORE_ASSERT(attachmentIndex < m_Attachments.size()); return m_Attachments[attachmentIndex]; }
		virtual Ref<HazelImage2D> GetDepthImage() const override { return Ref<HazelImage2D>(); /* m_DepthAttachment; */ }

		virtual RendererID GetColorAttachmentRendererID() const { return 0; }
		virtual RendererID GetDepthAttachmentRendererID() const { return 0; }

		const VkDescriptorImageInfo& GetVulkanDescriptorInfo() const { return m_DescriptorImageInfo; }
		VkRenderPass GetRenderPass() const { return m_RenderPass; }
		VkFramebuffer GetVulkanFramebuffer() const { return m_Framebuffer; }

		size_t GetColorAttachmentCount() const { return m_Attachments.size(); }
		const std::vector<VkClearValue>& GetVulkanClearValues() const { return m_ClearValues; }
		virtual const HazelFramebufferSpecification& GetSpecification() const override { return m_Specification; }

	private:
		HazelFramebufferSpecification m_Specification;
		RendererID m_RendererID = 0;
		uint32_t m_Width = 0, m_Height = 0;

		std::vector<Ref<HazelImage2D>> m_Attachments;
		std::vector<Ref<HazelImage2D>> m_AttachmentImages;
		Ref<HazelImage2D>m_DepthAttachmentImage;

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

		std::vector<std::function<void(Ref<HazelFramebuffer>)>> m_ResizeCallbacks;

	};

}
