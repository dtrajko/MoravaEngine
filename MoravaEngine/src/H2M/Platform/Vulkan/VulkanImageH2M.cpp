#include "VulkanImageH2M.h"

#include "VulkanContextH2M.h"
#include "VulkanRendererH2M.h"


namespace H2M
{

	static std::map<VkImage, RefH2M<VulkanImage2D_H2M>> s_ImageReferences;

	VulkanImage2D_H2M::VulkanImage2D_H2M(ImageSpecificationH2M specification)
		: m_Format(specification.Format), m_Width(specification.Width), m_Height(specification.Height)
	{
	}

	VulkanImage2D_H2M::VulkanImage2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height)
		: m_Format(format), m_Width(width), m_Height(height)
	{
	}

	VulkanImage2D_H2M::~VulkanImage2D_H2M()
	{
		if (m_Info.Image)
		{
			const VulkanImageInfoH2M& info = m_Info;
			// HazelRenderer::SubmitResourceFree([info, layerViews = m_PerLayerImageViews]() {});
			{
				const auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
				vkDestroyImageView(vulkanDevice, info.ImageView, nullptr);
				vkDestroySampler(vulkanDevice, info.Sampler, nullptr);

				for (auto& view : m_PerLayerImageViews)
				{
					if (view)
					{
						vkDestroyImageView(vulkanDevice, info.ImageView, nullptr);
					}
				}

				VulkanAllocatorVMA_H2M allocator(std::string("VulkanImage2D_H2M"));
				allocator.DestroyImage(info.Image, info.MemoryAlloc);
				s_ImageReferences.erase(info.Image);

				HZ_CORE_WARN("VulkanImage2D_H2M::Release ImageView = {0}", (const void*)info.ImageView);
			}
			m_PerLayerImageViews.clear();
		}
	}

	void VulkanImage2D_H2M::Invalidate()
	{
		// RefH2M<VulkanImage2D_H2M> instance = this;
		// HazelRenderer::Submit([instance]() mutable {});
		{
			RT_Invalidate();
		}
	}

	void VulkanImage2D_H2M::Release()
	{
		if (m_Info.Image == nullptr) return;

		// RefH2M<VulkanImage2D_H2M> instance = this;
		// HazelRenderer::SubmitResourceFree([info = m_Info, layerViews = m_PerLayerImageViews]() mutable {});
		{
			const auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
			vkDestroyImageView(vulkanDevice, m_Info.ImageView, nullptr);
			vkDestroySampler(vulkanDevice, m_Info.Sampler, nullptr);

			for (auto& view : m_PerLayerImageViews)
			{
				if (view)
				{
					vkDestroyImageView(vulkanDevice, view, nullptr);
				}
			}
			VulkanAllocatorVMA allocator(std::string("VulkanImage2D_H2M"));
			allocator.DestroyImage(m_Info.Image, m_Info.MemoryAlloc);
			s_ImageReferences.erase(m_Info.Image);
		}

		m_Info.Image = nullptr;
		m_Info.ImageView = nullptr;
		m_Info.Sampler = nullptr;
		m_PerLayerImageViews.clear();

		m_MipImageViews.clear();
	}

	void VulkanImage2D_H2M::RT_Invalidate()
	{
		HZ_CORE_VERIFY(m_Specification.Width > 0 && m_Specification.Height > 0);

		// Try release first if necessary
		Release();

		VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
		VulkanAllocatorVMA allocator(std::string("Image2D"));

		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT; // TODO: this (probably) shouldn't be implied
		if (m_Specification.Usage == ImageUsage::Attachment)
		{
			if (Utils::IsDepthFormat(m_Specification.Format))
			{
				usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			}
			else
			{
				usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			}
		}
		else if (m_Specification.Usage == ImageUsage::Texture)
		{
			usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		else if (m_Specification.Usage == ImageUsage::Storage)
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		if (m_Specification.Format == ImageFormatH2M::DEPTH24STENCIL8)
		{
			aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkFormat vulkanFormat = Utils::VulkanImageFormat(m_Specification.Format);

		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = vulkanFormat;
		imageCreateInfo.extent.width = m_Specification.Width;
		imageCreateInfo.extent.height = m_Specification.Height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = m_Specification.Mips;
		imageCreateInfo.arrayLayers = m_Specification.Layers;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		m_Info.MemoryAlloc = allocator.AllocateImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Info.Image);
		s_ImageReferences[m_Info.Image] = this;

		// Create a default image view
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.viewType = m_Specification.Layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vulkanFormat;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.subresourceRange = {};
		imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = m_Specification.Mips;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = m_Specification.Layers;
		imageViewCreateInfo.image = m_Info.Image;
		VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_Info.ImageView));

		// TODO: Renderer should contain some kind of sampler cache
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.maxAnisotropy = 1.0f;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.maxLod = 100.0f;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_Info.Sampler));

		if (m_Specification.Usage == ImageUsage::Storage)
		{
			// Transition image to GENERAL layout
			VkCommandBuffer commandBuffer = VulkanContextH2M::GetCurrentDevice()->GetCommandBuffer(true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_Specification.Mips;
			subresourceRange.layerCount = m_Specification.Layers;

			Utils::InsertImageMemoryBarrier(commandBuffer, m_Info.Image,
				0, 0,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				subresourceRange);

			VulkanContextH2M::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
		}

		UpdateDescriptor();
	}

	void VulkanImage2D_H2M::CreatePerLayerImageViews()
	{
	}

	void VulkanImage2D_H2M::RT_CreatePerLayerImageViews()
	{
	}

	void VulkanImage2D_H2M::RT_CreatePerSpecificLayerImageViews(const std::vector<uint32_t>& layerIndices)
	{
	}

	VkImageView VulkanImage2D_H2M::GetMipImageView(uint32_t mip)
	{
		if (m_MipImageViews.find(mip) == m_MipImageViews.end())
		{
			RefH2M<VulkanImage2D_H2M> instance = this;
			// HazelRenderer::Submit([instance, mip]() mutable {});
			{
				RT_GetMipImageView(mip);
			}
			return nullptr;
		}

		return m_MipImageViews.at(mip);
	}

	VkImageView VulkanImage2D_H2M::RT_GetMipImageView(uint32_t mip)
	{
		if (m_MipImageViews.find(mip) == m_MipImageViews.end())
		{
			VkDevice device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

			VkImageAspectFlags aspectMask = Utils::IsDepthFormat(m_Specification.Format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			if (m_Specification.Format == ImageFormatH2M::DEPTH24STENCIL8)
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

	void VulkanImage2D_H2M::UpdateDescriptor()
	{
		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		m_DescriptorImageInfo.imageView = m_Info.ImageView;
		m_DescriptorImageInfo.sampler = m_Info.Sampler;
	}

	namespace Utils {
 
		VkFormat VulkanImageFormat(ImageFormatH2M format)
		{
			switch (format)
			{
			case ImageFormatH2M::RED32F:   return VK_FORMAT_R32_SFLOAT;
			case ImageFormatH2M::RG16F:    return VK_FORMAT_R16G16_SFLOAT;
			case ImageFormatH2M::RG32F:    return VK_FORMAT_R32G32_SFLOAT;
			case ImageFormatH2M::RGBA:     return VK_FORMAT_R8G8B8A8_UNORM;
			case ImageFormatH2M::RGBA16F:  return VK_FORMAT_R32G32B32A32_SFLOAT; // should be VK_FORMAT_R16G16B16A16_SFLOAT
			case ImageFormatH2M::RGBA32F:  return VK_FORMAT_R32G32B32A32_SFLOAT;
			case ImageFormatH2M::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
			case ImageFormatH2M::DEPTH24STENCIL8: return VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
			}

			Log::GetLogger()->error("VulkanImageFormat: ImageFormatH2M not supported: '{0}'!", format);
			// H2M_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

	}

}
