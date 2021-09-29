#include "VulkanImage.h"

#include "VulkanContext.h"


namespace Hazel {

	VulkanImage2D::VulkanImage2D(HazelImageFormat format, uint32_t width, uint32_t height)
		: m_Format(format), m_Width(width), m_Height(height)
	{
	}

	VulkanImage2D::~VulkanImage2D()
	{
	}

	void VulkanImage2D::Invalidate()
	{
	}

	void VulkanImage2D::Release()
	{
		auto vulkanDevice = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDestroyImageView(vulkanDevice, m_Info.ImageView, nullptr);
		vkDestroyImage(vulkanDevice, m_Info.Image, nullptr);
		vkDestroySampler(vulkanDevice, m_Info.Sampler, nullptr);
		vkFreeMemory(vulkanDevice, m_Info.Memory, nullptr);
	}

	VkImageView VulkanImage2D::GetMipImageView(uint32_t mip)
	{
		if (m_MipImageViews.find(mip) == m_MipImageViews.end())
		{
			Ref<VulkanImage2D> instance = this;
			// HazelRenderer::Submit([instance, mip]() mutable {});
			{
				RT_GetMipImageView(mip);
			}
			return nullptr;
		}

		return m_MipImageViews.at(mip);
	}

	VkImageView VulkanImage2D::RT_GetMipImageView(uint32_t mip)
	{
		if (m_MipImageViews.find(mip) == m_MipImageViews.end())
		{
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

			VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			if (m_Specification.Format == HazelImageFormat::DEPTH24STENCIL8)
				aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

			m_PerLayerImageViews.resize(m_Specification.Layers);
			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = vulkanFormat;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.subresourceRange = {};
			imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
			imageViewCreateInfo.subresourceRange.baseMipLevel = mip;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.image = m_Info.Image;

			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_MipImageViews[mip]));
		}
		return m_MipImageViews.at(mip);
	}

	void VulkanImage2D::UpdateDescriptor()
	{
		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

}
