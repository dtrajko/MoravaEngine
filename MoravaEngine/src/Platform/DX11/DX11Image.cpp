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

	void VulkanImage2D::UpdateDescriptor()
	{
		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

}
