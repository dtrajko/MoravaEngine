#include "VulkanTextureH2M.h"

#include "VulkanContextH2M.h"
#include "VulkanImageH2M.h"
#include "VulkanRendererH2M.h"

#include "../stb_image.h"


namespace H2M {

	namespace Utils {

		static VkFormat TextureFormatToVkFormat(ImageFormatH2M format)
		{
			switch (format)
			{
				// case ImageFormatH2M::RGB: return VK_FORMAT_R8G8B8_UNORM;
				case ImageFormatH2M::RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
				// case ImageFormatH2M::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
				case ImageFormatH2M::RGBA16F: return VK_FORMAT_R32G32B32A32_SFLOAT;
				case ImageFormatH2M::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
			}
			Log::GetLogger()->error("TextureFormatToVkFormat: ImageFormatH2M '{0}' not supported!", format);
			H2M_CORE_ASSERT(false);
			return VK_FORMAT_UNDEFINED;
		}

		static uint32_t MipCount(uint32_t width, uint32_t height)
		{
			return (uint32_t)std::floor(std::log2(glm::min(width, height))) + 1;
		}

		static void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

	}

	static void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkImageSubresourceRange subresourceRange,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
	{
		// Create an image barrier object
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		// Source layouts (old)
		// Source access mask controls actions that have to be finished on the old layout
		// before it will be transitioned to the new layout
		switch (oldImageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			// Image layout is undefined (or does not matter)
			// Only valid as initial layout
			// No flags required, listed only for completeness
			imageMemoryBarrier.srcAccessMask = 0;
			break;

		case VK_IMAGE_LAYOUT_PREINITIALIZED:
			// Image is preinitialized
			// Only valid as initial layout for linear images, preserves memory contents
			// Make sure host writes have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image is a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image is a depth/stencil attachment
			// Make sure any writes to the depth/stencil buffer have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image is a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image is a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image is read by a shader
			// Make sure any shader reads from the image have been finished
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Target layouts (new)
		// Destination access mask controls the dependency for the new image layout
		switch (newImageLayout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			// Image will be used as a transfer destination
			// Make sure any writes to the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			// Image will be used as a transfer source
			// Make sure any reads from the image have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			// Image will be used as a color attachment
			// Make sure any writes to the color buffer have been finished
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			// Image layout will be used as a depth/stencil attachment
			// Make sure any writes to depth/stencil buffer have been finished
			imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// Image will be read in a shader (sampler, input attachment)
			// Make sure any writes to the image have been finished
			if (imageMemoryBarrier.srcAccessMask == 0)
			{
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
			}
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		default:
			// Other source layouts aren't handled (yet)
			break;
		}

		// Put barrier inside setup command buffer
		vkCmdPipelineBarrier(
			cmdbuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);
	}

	static void SetImageLayout(
		VkCommandBuffer cmdbuffer,
		VkImage image,
		VkImageAspectFlags aspectMask,
		VkImageLayout oldImageLayout,
		VkImageLayout newImageLayout,
		VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
	{
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;
		SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
	}

	VulkanTexture2D_H2M::VulkanTexture2D_H2M(const std::string& path, bool srgb, TextureWrapH2M wrap)
		: m_Path(path)
	{
		int width, height, channels;

		if (stbi_is_hdr(path.c_str()))
		{
			// m_ImageData.Data = (byte*)stbi_loadf(path.c_str(), &width, &height, &channels, 4);
			// m_ImageData.Size = width * height * 4 * sizeof(float);
			// m_Format = ImageFormatH2M::RGBA16F;
			m_ImageData.Data = (byte*)stbi_load(path.c_str(), &width, &height, &channels, 4);
			m_ImageData.Size = width * height * 4;
			m_Format = ImageFormatH2M::RGBA;
		}
		else
		{
			stbi_set_flip_vertically_on_load(1);
			m_ImageData.Data = stbi_load(path.c_str(), &width, &height, &channels, 4);
			m_ImageData.Size = width * height * 4;
			m_Format = ImageFormatH2M::RGBA;
		}

		if (!m_ImageData.Data)
		{
			// H2M_CORE_ASSERT(m_ImageData.Data, "Failed to load image!");
			Log::GetLogger()->error("Failed to load image '{0}'!", path);
			throw "Failed to load image '" + path + "'!";
		}

		m_Width = width;
		m_Height = height;
		m_Channels = channels;

		H2M_CORE_ASSERT(m_Format != ImageFormatH2M::None);

		//	if (channels != 4 && channels != 3) {
		//		H2M_CORE_ASSERT(channels == 4 || channels == 3);
		//		Log::GetLogger()->error("Invalid number of channels: '{0}'!", channels);
		//		return;
		//	}

		// H2M_CORE_ASSERT(channels == 4);

		//	Ref<VulkanTexture2D_H2M> instance = this;
		//	HazelRenderer::Submit([instance]() mutable
		//	{
		//		instance->Invalidate();
		//	});

		Invalidate();
	}

	VulkanTexture2D_H2M::VulkanTexture2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, TextureWrapH2M wrap)
		: m_Format(format)
	{
		m_Width = width;
		m_Height = height;

		// H2M_CORE_ASSERT(format == ImageFormat::RGBA);
		uint32_t size = width * height * 4;

		m_ImageData = BufferH2M::Copy(data, size);
		memcpy(m_ImageData.Data, data, m_ImageData.Size);

		//	Ref<VulkanTexture2D_H2M> instance = this;
		//	HazelRenderer::Submit([instance]() mutable
		//	{
		//		instance->Invalidate();
		//	});

		Invalidate();
	}

	VulkanTexture2D_H2M::VulkanTexture2D_H2M(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrapH2M wrap)
	{
		H2M_CORE_ASSERT(false);
	}

	VulkanTexture2D_H2M::~VulkanTexture2D_H2M()
	{
		// Ref<VulkanTexture2D_H2M> instance = this;
		// HazelRenderer::Submit([instance]() {});
		{
			auto vulkanDevice = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

			vkDestroyImageView(vulkanDevice, m_DescriptorImageInfo.imageView, nullptr);
			vkDestroyImage(vulkanDevice, m_VkImage, nullptr);
			vkDestroySampler(vulkanDevice, m_DescriptorImageInfo.sampler, nullptr);
			vkFreeMemory(vulkanDevice, m_DeviceMemory, nullptr);
		}
	}

	void VulkanTexture2D_H2M::Invalidate()
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkDeviceSize size = m_ImageData.Size;

		VkFormat format = Utils::TextureFormatToVkFormat(m_Format);
		uint32_t mipCount = GetMipLevelCount();

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// VkMemoryRequirements memoryRequirements = {};
		// memoryRequirements.size = size;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VulkanAllocatorH2M allocator(std::string("Texture2D"));

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice, &bufferCreateInfo, nullptr, &stagingBuffer));

		VkMemoryRequirements memoryRequirements = {};
		vkGetBufferMemoryRequirements(vulkanDevice, stagingBuffer, &memoryRequirements);
		allocator.Allocate(memoryRequirements, &stagingMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice, stagingBuffer, stagingMemory, 0));

		// Copy texture data into host local staging buffer
		uint8_t* destData;
		VK_CHECK_RESULT(vkMapMemory(vulkanDevice, stagingMemory, 0, memoryRequirements.size, 0, (void**)&destData));
		memcpy(destData, m_ImageData.Data, size);
		vkUnmapMemory(vulkanDevice, stagingMemory);

		// Create optimal tiled target image on the device
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipCount;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { m_Width, m_Height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VK_CHECK_RESULT(vkCreateImage(vulkanDevice, &imageCreateInfo, nullptr, &m_VkImage));

		vkGetImageMemoryRequirements(vulkanDevice, m_VkImage, &memoryRequirements);
		allocator.Allocate(memoryRequirements, &m_DeviceMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice, m_VkImage, m_DeviceMemory, 0));

		VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

		// Image memory barriers for the texture image

		// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
		VkImageSubresourceRange subresourceRange = {};
		// Image only contains color data
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// Start at first mip level
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
		Utils::InsertImageMemoryBarrier(copyCmd, m_VkImage,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			subresourceRange);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = m_Width;
		bufferCopyRegion.imageExtent.height = m_Height;
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = 0;

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			m_VkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion);

		Utils::InsertImageMemoryBarrier(copyCmd, m_VkImage,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			subresourceRange);

		device->FlushCommandBuffer(copyCmd);

		// Clean up staging resources
		vkFreeMemory(vulkanDevice, stagingMemory, nullptr);
		vkDestroyBuffer(vulkanDevice, stagingBuffer, nullptr);

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CREATE TEXTURE SAMPLER
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create a texture sampler
		VkSamplerCreateInfo sampler{};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.maxAnisotropy = 1.0f;
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		sampler.maxLod = (float)mipCount;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device

		// TODO:
		/*if (vulkanDevice->features.samplerAnisotropy) {
			// Use max. level of anisotropy for this example
			sampler.maxAnisotropy = 1.0f;// vulkanDevice->properties.limits.maxSamplerAnisotropy;
			sampler.anisotropyEnable = VK_TRUE;
		}
		else {
			// The device does not support anisotropic filtering
			sampler.maxAnisotropy = 1.0;
			sampler.anisotropyEnable = VK_FALSE;
		}*/
		sampler.maxAnisotropy = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &sampler, nullptr, &m_DescriptorImageInfo.sampler));

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		view.subresourceRange.levelCount = mipCount;
		view.image = m_VkImage;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView));

		GenerateMips();

		// Store current layout for later reuse
		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	void VulkanTexture2D_H2M::Bind(uint32_t slot) const
	{
	}

	void VulkanTexture2D_H2M::Lock()
	{
	}

	void VulkanTexture2D_H2M::Unlock()
	{
	}

	// void VulkanTexture2D_H2M::Resize(uint32_t width, uint32_t height) {} // method removed in Hazel Live 18.03.2021 #2

	BufferH2M VulkanTexture2D_H2M::GetWriteableBuffer()
	{
		return m_ImageData;
	}

	bool VulkanTexture2D_H2M::Loaded() const
	{
		return true;
	}

	const std::string& VulkanTexture2D_H2M::GetPath() const
	{
		return m_Path;
	}

	/**** BEGIN removed in Hazel Live 18.03.2021 #2 ****
	ImageFormatH2M VulkanTexture2D_H2M::GetFormat() const
	{
		return m_Format;
	}
	/**** END removed in Hazel Live 18.03.2021 #2 ****/

	uint32_t VulkanTexture2D_H2M::GetMipLevelCount() const
	{
		return Utils::MipCount(m_Width, m_Height);
	}

	std::pair<uint32_t, uint32_t> VulkanTexture2D_H2M::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("VulkanTexture2D_H2M::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

	void VulkanTexture2D_H2M::GenerateMips()
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandBuffer blitCmd = VulkanContextH2M::GetCurrentDevice()->GetCommandBuffer(true);

		// Base image barrier
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_VkImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		// Copy down mips from n-1 to n
		int32_t mipLevels = GetMipLevelCount();
		for (int32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit imageBlit{};

			// Source
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(m_Width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(m_Height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			// Destination
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(m_Width >> i);
			imageBlit.dstOffsets[1].y = int32_t(m_Height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			Utils::InsertImageMemoryBarrier(blitCmd, m_VkImage,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);

			// Blit from previous level
			vkCmdBlitImage(
				blitCmd,
				m_VkImage,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_VkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&imageBlit,
				VK_FILTER_LINEAR);

			// Prepare current mip level as image blit source for next level
			Utils::InsertImageMemoryBarrier(blitCmd, m_VkImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = mipLevels;

		Utils::InsertImageMemoryBarrier(blitCmd, m_VkImage,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		VulkanContextH2M::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

		m_MipsGenerated = true;
	}

	//////////////////////////////////////////////////////////////////////////////////
	// TextureCube
	//////////////////////////////////////////////////////////////////////////////////

	VulkanTextureCube::VulkanTextureCube(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
		: m_Format(format), m_Width(width), m_Height(height)
	{
		if (data)
		{
			uint32_t size = width * height * 4 * 6; // six layers
			m_LocalStorage = BufferH2M::Copy(data, size);
		}

		// Ref<VulkanTextureCube> instance = this;
		// HazelRenderer::Submit([instance]() mutable { instance->Invalidate(); });

		Invalidate();
	}

	VulkanTextureCube::VulkanTextureCube(const std::string& path)
	{
		Log::GetLogger()->error("VulkanTextureCube::VulkanTextureCube('{0}') - method not implemented!", path);
	}

	VulkanTextureCube::~VulkanTextureCube()
	{
	}

	void VulkanTextureCube::Invalidate()
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		// VkDeviceSize size = m_Width * m_Height * 4 * 2;

		VkFormat format = Utils::TextureFormatToVkFormat(m_Format);
		uint32_t mipCount = GetMipLevelCount();

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		// VkMemoryRequirements memoryRequirements = {};
		// memoryRequirements.size = size;

		VulkanAllocatorH2M allocator(std::string("TextureCube"));

		// Create optimal tiled target image on the device
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipCount; // TODO
		imageCreateInfo.arrayLayers = 6;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.extent = { m_Width, m_Height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		VK_CHECK_RESULT(vkCreateImage(vulkanDevice, &imageCreateInfo, nullptr, &m_Image));

		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements(vulkanDevice, m_Image, &memoryRequirements);
		allocator.Allocate(memoryRequirements, &m_DeviceMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice, m_Image, m_DeviceMemory, 0));

		m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkCommandBuffer layoutCmd = device->GetCommandBuffer(true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipCount;
		subresourceRange.layerCount = 6;

		SetImageLayout(
			layoutCmd, m_Image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			m_DescriptorImageInfo.imageLayout,
			subresourceRange);

		device->FlushCommandBuffer(layoutCmd);

		m_MipsGenerated = true;

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// CREATE TEXTURE SAMPLER
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Create a texture sampler
		VkSamplerCreateInfo sampler{};
		sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		sampler.maxAnisotropy = 1.0f;
		sampler.magFilter = VK_FILTER_LINEAR;
		sampler.minFilter = VK_FILTER_LINEAR;
		sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		sampler.mipLodBias = 0.0f;
		sampler.compareOp = VK_COMPARE_OP_NEVER;
		sampler.minLod = 0.0f;
		// Set max level-of-detail to mip level count of the texture
		sampler.maxLod = (float)mipCount;
		// Enable anisotropic filtering
		// This feature is optional, so we must check if it's supported on the device

		sampler.maxAnisotropy = 1.0;
		sampler.anisotropyEnable = VK_FALSE;
		sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &sampler, nullptr, &m_DescriptorImageInfo.sampler));

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// information and sub resource ranges
		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = mipCount;
		view.image = m_Image;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView));
	}

	uint32_t VulkanTextureCube::GetMipLevelCount() const
	{
		return VulkanRendererH2M::s_MipMapsEnabled ? Utils::MipCount(m_Width, m_Height) : 1;
	}

	VkImageView VulkanTextureCube::CreateImageViewSingleMip(uint32_t mip)
	{
		// TODO: assert to check mip count

		auto device = VulkanContextH2M::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkFormat format = Utils::VulkanImageFormat(m_Format);

		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
		view.format = format;
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = mip;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 6;
		view.subresourceRange.levelCount = 1;
		view.image = m_Image;

		VkImageView result;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &result));
		return result;
	}

	void VulkanTextureCube::GenerateMips(bool readonly)
	{
		auto device = VulkanContextH2M::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkCommandBuffer blitCmd = VulkanContextH2M::GetCurrentDevice()->GetCommandBuffer(true);

		uint32_t mipLevels = GetMipLevelCount();
		for (uint32_t face = 0; face < 6; face++)
		{
			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = 0;
			mipSubRange.baseArrayLayer = face;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mipSubRange);
		}

		// Copy down mips from n-1 to n
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t face = 0; face < 6; face++)
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcSubresource.baseArrayLayer = face;
				imageBlit.srcOffsets[1].x = int32_t(m_Width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(m_Height >> (i - 1));
				imageBlit.srcOffsets[1].z = 1;

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstSubresource.baseArrayLayer = face;
				imageBlit.dstOffsets[1].x = int32_t(m_Width >> i);
				imageBlit.dstOffsets[1].y = int32_t(m_Height >> i);
				imageBlit.dstOffsets[1].z = 1;

				Log::GetLogger()->info("VTCube::GenerateMips mipLevel: {0}, cube face: {1}, src: {2}x{3}, dst: {4}x{5}",
					i, face, int32_t(m_Width >> (i - 1)), int32_t(m_Height >> (i - 1)), int32_t(m_Width >> i), int32_t(m_Height >> i));

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.baseArrayLayer = face;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				// Prepare current mip level as image blit destination
				Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
					0, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);

				// Blit from previous level
				vkCmdBlitImage(
					blitCmd,
					m_Image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlit,
					VK_FILTER_LINEAR);

				// Prepare current mip level as image blit source for next level
				Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					mipSubRange);

			}
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 6;
		subresourceRange.levelCount = mipLevels;

		Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			subresourceRange);

		if (readonly)
		{
			Utils::InsertImageMemoryBarrier(blitCmd, m_Image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				// VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);

			SetImageLayout(
				blitCmd, m_Image,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				subresourceRange,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		VulkanContextH2M::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

		m_MipsGenerated = true;

		m_DescriptorImageInfo.imageLayout = readonly ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;

#if 0
		auto device = VulkanContext::GetCurrentDevice();
		auto vulkanDevice = device->GetVulkanDevice();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = m_Image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = m_Width;
		int32_t mipHeight = m_Height;

		VkCommandBuffer blitCmd = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

		auto mipLevels = GetMipLevelCount();
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			for (uint32_t face = 0; face < 6; face++)
			{
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.subresourceRange.baseArrayLayer = face;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				
				vkCmdPipelineBarrier(blitCmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);

				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = face;
				blit.srcSubresource.layerCount = 1;
				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1};
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = face;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(blitCmd,
					m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(blitCmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier);
			}

			if (mipWidth > 1)
				mipWidth /= 2;
			if (mipHeight > 1)
				mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(blitCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		SetImageLayout(
			blitCmd, m_Image,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			barrier.subresourceRange,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		VulkanContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);
#endif

	}

	std::pair<uint32_t, uint32_t> VulkanTextureCube::GetMipSize(uint32_t mip) const
	{
		Log::GetLogger()->error("VulkanTextureCube::GetMipSize({0}) - method not implemented!", mip);
		return std::pair<uint32_t, uint32_t>();
	}

}
