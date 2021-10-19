#include "VulkanMoravaTexture.h"

#include "Core/Log.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Platform/Vulkan/Vulkan.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanImage.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"

#include <fstream>
#include <exception>
#include <string>


namespace Utils
{
	// Hazel::VulkanImage
	VkFormat VulkanImageFormat(Hazel::HazelImageFormat format)
	{
		switch (format)
		{
		case Hazel::HazelImageFormat::RED32F:   return VK_FORMAT_R32_SFLOAT;
		case Hazel::HazelImageFormat::RG16F:    return VK_FORMAT_R16G16_SFLOAT;
		case Hazel::HazelImageFormat::RG32F:    return VK_FORMAT_R32G32_SFLOAT;
		case Hazel::HazelImageFormat::RGBA:     return VK_FORMAT_R8G8B8A8_UNORM;
		case Hazel::HazelImageFormat::RGBA16F:  return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Hazel::HazelImageFormat::RGBA32F:  return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Hazel::HazelImageFormat::DEPTH32F: return VK_FORMAT_D32_SFLOAT;
		case Hazel::HazelImageFormat::DEPTH24STENCIL8: return Hazel::VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();
		}

		Log::GetLogger()->error("VulkanImageFormat: HazelImageFormat not supported: '{0}'!", format);
		// HZ_CORE_ASSERT(false);
		return VK_FORMAT_UNDEFINED;
	}

	static VkSamplerAddressMode VulkanSamplerWrap(Hazel::TextureWrap wrap)
	{
		switch (wrap)
		{
		case Hazel::TextureWrap::Clamp:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case Hazel::TextureWrap::Repeat:  return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}
		HZ_CORE_ASSERT(false, "Unknown wrap mode");
		return (VkSamplerAddressMode)0;
	}

	static VkFilter VulkanSamplerFilter(Hazel::TextureFilter filter)
	{
		switch (filter)
		{
		case Hazel::TextureFilter::Linear:   return VK_FILTER_LINEAR;
		case Hazel::TextureFilter::Nearest:  return VK_FILTER_NEAREST;
		}
		HZ_CORE_ASSERT(false, "Unknown filter");
		return (VkFilter)0;
	}

}

VulkanMoravaTexture::VulkanMoravaTexture()
{
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_Spec.InternalFormat = 0;
	m_Spec.Border = 0;
	m_Spec.Format = 0;
	// m_Spec.Type = GL_UNSIGNED_BYTE;
	m_Spec.FlipVertically = false;
	m_Spec.IsSampler = false;
	// m_Spec.Texture_Wrap_S = GL_REPEAT;
	// m_Spec.Texture_Wrap_T = GL_REPEAT;
	// m_Spec.Texture_Wrap_R = GL_REPEAT;
	// m_Spec.Texture_Min_Filter = GL_LINEAR;
	// m_Spec.Texture_Mag_Filter = GL_LINEAR;
	m_Spec.MipLevel = 0;

	m_Level = 0;
	m_ID = 0;
	m_FileLocation = "";
	m_Buffer = nullptr;
	m_Format = Hazel::HazelImageFormat::RGBA;
}

/**
 * Implementation from Hazel/Platform/Vulkan/VulkanTexture class
 * VulkanTexture2D(const std::string& path, bool srgb = false, TextureWrap wrap = TextureWrap::Clamp);
 **/
VulkanMoravaTexture::VulkanMoravaTexture(const char* fileLoc, bool flipVert, bool isSampler, int filter)
	: VulkanMoravaTexture()
{
	m_FileLocation = fileLoc;

	int width, height, channels;
	stbi_set_flip_vertically_on_load(1);
	m_ImageData.Data = stbi_load(fileLoc, &width, &height, &channels, 4);
	m_ImageData.Size = width * height * 4;
	if (!m_ImageData.Data)
	{
		Log::GetLogger()->error("VulkanMoravaTexture: Failed to load image!");
	}
	m_Width = width;
	m_Height = height;
	m_Channels = channels;

	if (channels != 4 && channels != 3) {
		return;
	}

	HZ_CORE_ASSERT(channels == 4 || channels == 3);
	// HZ_CORE_ASSERT(channels == 4);

	//	Ref<VulkanTexture2D> instance = this;
	//	HazelRenderer::Submit([instance]() mutable
	//	{
	//		instance->Invalidate();
	//	});

	Invalidate();
}

VulkanMoravaTexture::VulkanMoravaTexture(const char* fileLoc, uint32_t width, uint32_t height, bool isSampler, int filter)
	: VulkanMoravaTexture()
{
	m_FileLocation = fileLoc;
	m_Spec.Width = width;
	m_Spec.Height = height;
	m_Spec.IsSampler = isSampler;
	m_Spec.BitDepth = 4;
	m_Spec.Texture_Min_Filter = filter;
	m_Spec.Texture_Mag_Filter = filter;

	m_Buffer = new unsigned char[m_Spec.Width * m_Spec.Height * m_Spec.BitDepth];

	if (!m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}
}

/**
* Constructor for a fully customizable 2D texture
*/
VulkanMoravaTexture::VulkanMoravaTexture(const char* fileLoc, Specification spec)
	: VulkanMoravaTexture()
{
	m_FileLocation = fileLoc;

	m_Spec.InternalFormat     = spec.InternalFormat;
	m_Spec.Width              = spec.Width;
	m_Spec.Height             = spec.Height;
	m_Spec.Border             = spec.Border;
	m_Spec.Format             = spec.Format;
	m_Spec.Type               = spec.Type;
	m_Spec.Texture_Wrap_S     = spec.Texture_Wrap_S;
	m_Spec.Texture_Wrap_T     = spec.Texture_Wrap_T;
	m_Spec.Texture_Wrap_R     = spec.Texture_Wrap_R;
	m_Spec.Texture_Min_Filter = spec.Texture_Min_Filter;
	m_Spec.Texture_Mag_Filter = spec.Texture_Mag_Filter;
	m_Spec.MipLevel           = spec.MipLevel;
	m_Spec.FlipVertically     = spec.FlipVertically;
	m_Spec.BitDepth           = spec.BitDepth;
	m_Spec.IsSampler          = spec.IsSampler;
	m_Spec.Samples            = spec.Samples;
	m_Spec.IsMultisample      = spec.IsMultisample;
	m_Spec.IsSRGB             = spec.IsSRGB;

	Load(m_Spec.FlipVertically);
}

/**
 * Invalidate method is copied from the Hazel/Platform/Vulkan/VulkanTexture class
 */
void VulkanMoravaTexture::Invalidate()
{
	auto device = Hazel::VulkanContext::GetCurrentDevice();
	auto vulkanDevice = device->GetVulkanDevice();

	m_Image->Release();

	uint32_t mipCount = m_Properties.GenerateMips ? GetMipLevelCount() : 1;

	Hazel::ImageSpecification& imageSpec = m_Image->GetSpecification();
	imageSpec.Format = m_Format;
	imageSpec.Width = m_Width;
	imageSpec.Height = m_Height;
	imageSpec.Mips = mipCount;
	if (!m_ImageData) // TODO(Yan): better management for this, probably from texture spec
	{
		imageSpec.Usage = Hazel::ImageUsage::Storage;
	}

	Hazel::Ref<Hazel::VulkanImage2D> image = m_Image.As<Hazel::VulkanImage2D>();
	image->RT_Invalidate();

	auto& info = image->GetImageInfo();

	if (m_ImageData)
	{
		VkDeviceSize size = m_ImageData.Size;

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		Hazel::VulkanAllocator allocator("Texture2D");

		// Create staging buffer
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation = allocator.AllocateBuffer(bufferCreateInfo, VMA_MEMORY_USAGE_CPU_TO_GPU, stagingBuffer);

		// Copy data to staging buffer
		uint8_t* destData = allocator.MapMemory<uint8_t>(stagingBufferAllocation);
		HZ_CORE_ASSERT(m_ImageData.Data);
		memcpy(destData, m_ImageData.Data, size);
		allocator.UnmapMemory(stagingBufferAllocation);

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
		VkImageMemoryBarrier imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = info.Image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
		// Source pipeline stage is host write/read exection (VK_PIPELINE_STAGE_HOST_BIT)
		// Destination pipeline stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_HOST_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

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
			info.Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferCopyRegion);

#if 0
		// Once the data has been uploaded we transfer to the texture image to the shader read layout, so it can be sampled from
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Insert a memory dependency at the proper pipeline stages that will execute the image layout transition 
		// Source pipeline stage stage is copy command exection (VK_PIPELINE_STAGE_TRANSFER_BIT)
		// Destination pipeline stage fragment shader access (VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
		vkCmdPipelineBarrier(
			copyCmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

#endif

		if (mipCount > 1) // Mips to generate
		{
			Hazel::Utils::InsertImageMemoryBarrier(copyCmd, info.Image,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				subresourceRange);
		}
		else
		{
			Hazel::Utils::InsertImageMemoryBarrier(copyCmd, info.Image,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image->GetDescriptor().imageLayout,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				subresourceRange);
		}

		device->FlushCommandBuffer(copyCmd);

		// Clean up staging resources
		allocator.DestroyBuffer(stagingBuffer, stagingBufferAllocation);
	}
	else
	{
		VkCommandBuffer transitionCommandBuffer = device->GetCommandBuffer(true);
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.layerCount = 1;
		subresourceRange.levelCount = GetMipLevelCount();
		Hazel::Utils::SetImageLayout(transitionCommandBuffer, info.Image, VK_IMAGE_LAYOUT_UNDEFINED, image->GetDescriptor().imageLayout, subresourceRange);
		device->FlushCommandBuffer(transitionCommandBuffer);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CREATE TEXTURE SAMPLER
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create a texture sampler
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;
	sampler.magFilter = Utils::VulkanSamplerFilter(m_Properties.SamplerFilter);
	sampler.minFilter = Utils::VulkanSamplerFilter(m_Properties.SamplerFilter);
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = Utils::VulkanSamplerWrap(m_Properties.SamplerWrap);
	sampler.addressModeV = Utils::VulkanSamplerWrap(m_Properties.SamplerWrap);
	sampler.addressModeW = Utils::VulkanSamplerWrap(m_Properties.SamplerWrap);
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
	VK_CHECK_RESULT(vkCreateSampler(vulkanDevice, &sampler, nullptr, &info.Sampler));

	if (!m_Properties.Storage)
	{
		VkImageViewCreateInfo view{};
		view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view.format = Utils::VulkanImageFormat(m_Format);
		view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		// The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
		// It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
		view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view.subresourceRange.baseMipLevel = 0;
		view.subresourceRange.baseArrayLayer = 0;
		view.subresourceRange.layerCount = 1;
		view.subresourceRange.levelCount = mipCount;
		view.image = info.Image;
		VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &info.ImageView));


		image->UpdateDescriptor();
	}

	if (m_ImageData && m_Properties.GenerateMips && mipCount > 1)
	{
		GenerateMips();
	}
}

bool VulkanMoravaTexture::Load(bool flipVert)
{
	if (m_Buffer)
	{
		Log::GetLogger()->info("Texture '{0}' already loaded. Skipping...", m_FileLocation);
		return true;
	}

	stbi_set_flip_vertically_on_load(flipVert ? 1 : 0);

	if (stbi_is_hdr(m_FileLocation))
	{
		Log::GetLogger()->info("Loading an HDR texture '{0}'", m_FileLocation);
		m_Buffer = (byte*)stbi_loadf(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = Hazel::HazelImageFormat::RGBA16F;
	}
	else
	{
		m_Buffer = stbi_load(m_FileLocation, (int*)&m_Spec.Width, (int*)&m_Spec.Height, &m_Spec.BitDepth, 0);
		m_Format = Hazel::HazelImageFormat::RGBA;
	}

	if (!m_Buffer)
	{
		std::string message = "ERROR: Texture failed to load '" + std::string(m_FileLocation) + "'";
		throw std::runtime_error(message.c_str());
	}

	CreateAPISpecific();

	float fileSize = GetFileSize(m_FileLocation) / (1024.0f * 1024.0f);
	Log::GetLogger()->info("Loading texture '{0}' [ID={1}, size={2} MB]", m_FileLocation, m_ID, fileSize);

	if (!m_Spec.IsSampler)
		stbi_image_free(m_Buffer);

	return true;
}

void VulkanMoravaTexture::CreateAPISpecific()
{
	Log::GetLogger()->error("VulkanMoravaTexture::CreateAPISpecific method not yet implemented!");
}

void VulkanMoravaTexture::Save()
{
	CreateAPISpecific();
	stbi_write_png(m_FileLocation, m_Spec.Width, m_Spec.Height, m_Spec.BitDepth, m_Buffer, m_Spec.Width * m_Spec.BitDepth);
}

int VulkanMoravaTexture::GetRed(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0];
}

int VulkanMoravaTexture::GetGreen(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1];
}

int VulkanMoravaTexture::GetBlue(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2];

}

int VulkanMoravaTexture::GetAlpha(int x, int z)
{
	return (int)m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3];
}

void VulkanMoravaTexture::SetPixel(int x, int z, glm::ivec4 pixel)
{
	SetRed(  x, z, pixel.x);
	SetGreen(x, z, pixel.y);
	SetBlue( x, z, pixel.z);
	SetAlpha(x, z, pixel.w);
}

void VulkanMoravaTexture::SetRed(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 0] = value;
}

void VulkanMoravaTexture::SetGreen(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 1] = value;
}

void VulkanMoravaTexture::SetBlue(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 2] = value;
}

void VulkanMoravaTexture::SetAlpha(int x, int z, int value)
{
	m_Buffer[((z * m_Spec.Width + x) * m_Spec.BitDepth) + 3] = value;
}

void VulkanMoravaTexture::GenerateMips(bool readonly)
{
	auto device = Hazel::VulkanContext::GetCurrentDevice();
	auto vulkanDevice = device->GetVulkanDevice();

	Hazel::Ref<Hazel::VulkanImage2D> image = m_Image.As<Hazel::VulkanImage2D>();
	const auto& info = image->GetImageInfo();

	const VkCommandBuffer blitCmd = Hazel::VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = info.Image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	const auto mipLevels = GetMipLevelCount();
	for (uint32_t i = 1; i < mipLevels; i++)
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
		Hazel::Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			mipSubRange);

		// Blit from previous level
		vkCmdBlitImage(
			blitCmd,
			info.Image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			info.Image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlit,
			VK_FILTER_LINEAR);

		// Prepare current mip level as image blit source for next level
		Hazel::Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
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

	Hazel::Utils::InsertImageMemoryBarrier(blitCmd, info.Image,
		VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		subresourceRange);

	Hazel::VulkanContext::GetCurrentDevice()->FlushCommandBuffer(blitCmd);

#if 0
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_Image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;
	barrier.subresourceRange = subresourceRange;

	int32_t mipWidth = m_Width;
	int32_t mipHeight = m_Height;

	VkCommandBuffer commandBuffer = VulkanContext::GetCurrentDevice()->GetCommandBuffer(true);

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	auto mipLevels = GetMipLevelCount();
	for (uint32_t i = 1; i < mipLevels; i++)
	{
		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.subresourceRange.baseMipLevel = i;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	// Transition all mips from transfer to shader read
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	VulkanContext::GetCurrentDevice()->FlushCommandBuffer(commandBuffer);
#endif
}

void VulkanMoravaTexture::Bind(uint32_t textureSlot) const
{
	Log::GetLogger()->error("VulkanMoravaTexture::Bind - method not yet implemented!");
}

void VulkanMoravaTexture::Unbind()
{
	Log::GetLogger()->error("VulkanMoravaTexture::Unbind - method not yet implemented!");
}

float VulkanMoravaTexture::GetFileSize(const char* filename)
{
	struct stat stat_buf;
	int rc = stat(filename, &stat_buf);
	return rc == 0 ? (float) stat_buf.st_size : -1.0f;
}

void VulkanMoravaTexture::Clear()
{
	if (m_Spec.IsSampler)
	{
		stbi_image_free(m_Buffer);
	}

	m_ID = 0;
	m_Spec.Width = 0;
	m_Spec.Height = 0;
	m_Spec.BitDepth = 0;
	m_FileLocation = "";
}

uint32_t VulkanMoravaTexture::CalculateMipMapCount(uint32_t width, uint32_t height)
{
	uint32_t levels = 1;
	while ((width | height) >> levels)
	{
		levels++;
	}

	return levels;
}

uint32_t VulkanMoravaTexture::GetMipLevelCount()
{
	return CalculateMipMapCount(m_Spec.Width, m_Spec.Height);
}

std::pair<uint32_t, uint32_t> VulkanMoravaTexture::GetMipSize(uint32_t mip) const
{
	Log::GetLogger()->error("VulkanMoravaTexture::GetMipSize({0}) - method not implemented!", mip);
	return std::pair<uint32_t, uint32_t>();
}

VulkanMoravaTexture::~VulkanMoravaTexture()
{
	Clear();
}
