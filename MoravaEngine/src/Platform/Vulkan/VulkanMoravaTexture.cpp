#include "VulkanMoravaTexture.h"

#include "Core/Log.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Platform/Vulkan/Vulkan.h"
#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Platform/Vulkan/VulkanImage.h"
#include "Hazel/Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/HazelImage.h"

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

	VkDeviceSize size = m_ImageData.Size;

	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memoryRequirements = {};
	memoryRequirements.size = size;

	// Copy data to an optimal tiled image
	// This loads the texture data into a host local buffer that is copied to the optimal tiled image on the device

	// Create a host-visible staging buffer that contains the raw image data
	// This buffer will be the data source for copying texture data to the optimal tiled image on the device
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	Hazel::VulkanAllocator allocator(std::string("Texture2D"));

	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK_RESULT(vkCreateBuffer(vulkanDevice, &bufferCreateInfo, nullptr, &stagingBuffer));
	vkGetBufferMemoryRequirements(vulkanDevice, stagingBuffer, &memoryRequirements);
	allocator.Allocate(memoryRequirements, &stagingMemory, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK_RESULT(vkBindBufferMemory(vulkanDevice, stagingBuffer, stagingMemory, 0));

	// Copy texture data into host local staging buffer
	uint8_t* destData;
	VK_CHECK_RESULT(vkMapMemory(vulkanDevice, stagingMemory, 0, memoryRequirements.size, 0, (void**)&destData));
	memcpy(destData, m_ImageData.Data, size);
	vkUnmapMemory(vulkanDevice, stagingMemory);

	/*
	// Setup buffer copy regions for each mip level
	std::vector<VkBufferImageCopy> bufferCopyRegions;
	uint32_t offset = 0;

	for (uint32_t i = 0; i < texture.mipLevels; i++) {
		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = i;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2D[i].extent().x);
		bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2D[i].extent().y);
		bufferCopyRegion.imageExtent.depth = 1;
		bufferCopyRegion.bufferOffset = offset;

		bufferCopyRegions.push_back(bufferCopyRegion);

		offset += static_cast<uint32_t>(tex2D[i].size());
	}
	*/

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.mipLevel = 0;
	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = m_Width;
	bufferCopyRegion.imageExtent.height = m_Height;
	bufferCopyRegion.imageExtent.depth = 1;
	bufferCopyRegion.bufferOffset = 0;

	// Create optimal tiled target image on the device
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// Set initial layout of the image to undefined
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { m_Width, m_Height, 1 };
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VK_CHECK_RESULT(vkCreateImage(vulkanDevice, &imageCreateInfo, nullptr, &m_Image));

	vkGetImageMemoryRequirements(vulkanDevice, m_Image, &memoryRequirements);
	allocator.Allocate(memoryRequirements, &m_DeviceMemory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice, m_Image, m_DeviceMemory, 0));

	VkCommandBuffer copyCmd = device->GetCommandBuffer(true);

	// Image memory barriers for the texture image

	// The sub resource range describes the regions of the image that will be transitioned using the memory barriers below
	VkImageSubresourceRange subresourceRange = {};
	// Image only contains color data
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	// Start at first mip level
	subresourceRange.baseMipLevel = 0;
	// We will transition on all mip levels
	subresourceRange.levelCount = 1; // TODO: Support mips
	// The 2D texture only has one layer
	subresourceRange.layerCount = 1;

	// Transition the texture image layout to transfer target, so we can safely copy our buffer data to it.
	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = m_Image;
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

	// Copy mip levels from staging buffer
	vkCmdCopyBufferToImage(
		copyCmd,
		stagingBuffer,
		m_Image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&bufferCopyRegion);

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

	// Store current layout for later reuse
	m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	device->FlushCommandBuffer(copyCmd);

	// Clean up staging resources
	vkFreeMemory(vulkanDevice, stagingMemory, nullptr);
	vkDestroyBuffer(vulkanDevice, stagingBuffer, nullptr);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CREATE TEXTURE SAMPLER
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Create a texture sampler
	// In Vulkan textures are accessed by samplers
	// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
	// Note: Similar to the samplers available with OpenGL 3.3
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	// Set max level-of-detail to mip level count of the texture
	sampler.maxLod = 1.0f;
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
	// Linear tiling usually won't support mip maps
	// Only set mip map count if optimal tiling is used
	view.subresourceRange.levelCount = 1;
	// The view will be based on the texture's image
	view.image = m_Image;
	VK_CHECK_RESULT(vkCreateImageView(vulkanDevice, &view, nullptr, &m_DescriptorImageInfo.imageView));
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
