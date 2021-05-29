#include "VulkanFramebuffer.h"

#include "VulkanContext.h"
#include "VulkanTexture.h"

#include "Core/Application.h"


namespace Hazel {

	namespace Utils {

		static bool IsDepthFormat(HazelImageFormat format)
		{
			switch (format)
			{
			case HazelImageFormat::DEPTH24STENCIL8:
			case HazelImageFormat::DEPTH32F:
				return true;
			}
			return false;
		}

	}

	VulkanFramebuffer::VulkanFramebuffer(const HazelFramebufferSpecification& spec)
		: m_Specification(spec)
	{
		Resize(spec.Width, spec.Height, true);
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		m_Specification.Width = width;
		m_Specification.Height = height;

		if (!m_Specification.SwapChainTarget)
		{
			Ref<VulkanFramebuffer> instance = this;
			//	HazelRenderer::Submit([instance, width, height]() mutable
			//	{
			//	});
			{
				auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

				if (instance->m_Framebuffer) {
					vkDestroyFramebuffer(device, instance->m_Framebuffer, nullptr);

					VulkanAllocator allocator(std::string("Framebuffer"));

					std::array<VkAttachmentDescription, 2> attachmentDescriptions;

					// COLOR ATTACHMENT
					{
						const VkFormat COLOR_BUFFER_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

						VkImageCreateInfo imageCreateInfo = {};
						imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
						imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
						imageCreateInfo.format = COLOR_BUFFER_FORMAT;
						imageCreateInfo.extent.width = width;
						imageCreateInfo.extent.height = height;
						imageCreateInfo.extent.depth = 1;
						imageCreateInfo.mipLevels = 1;
						imageCreateInfo.arrayLayers = 1;
						imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
						imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
						// We will sample directly from the color attachment
						imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

						VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &instance->m_ColorAttachment.image));

						VkMemoryRequirements memReqs;
						vkGetImageMemoryRequirements(device, instance->m_ColorAttachment.image, &memReqs);

						allocator.Allocate(memReqs, &instance->m_ColorAttachment.mem);

						VK_CHECK_RESULT(vkBindImageMemory(device, instance->m_ColorAttachment.image, instance->m_ColorAttachment.mem, 0));

						VkImageViewCreateInfo colorImageViewCreateInfo = {};
						colorImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
						colorImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
						colorImageViewCreateInfo.format = COLOR_BUFFER_FORMAT;
						colorImageViewCreateInfo.subresourceRange = {};
						colorImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						colorImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
						colorImageViewCreateInfo.subresourceRange.levelCount = 1;
						colorImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
						colorImageViewCreateInfo.subresourceRange.layerCount = 1;
						colorImageViewCreateInfo.image = instance->m_ColorAttachment.image;
						VK_CHECK_RESULT(vkCreateImageView(device, &colorImageViewCreateInfo, nullptr, &instance->m_ColorAttachment.view));

						// Create sampler to sample from the attachment in the fragment shader
						VkSamplerCreateInfo samplerCreateInfo = {};
						samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
						samplerCreateInfo.maxAnisotropy = 1.0f;
						samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
						samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
						samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
						samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						samplerCreateInfo.mipLodBias = 0.0f;
						samplerCreateInfo.maxAnisotropy = 1.0f;
						samplerCreateInfo.minLod = 0.0f;
						samplerCreateInfo.maxLod = 1.0f;
						samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
						VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &instance->m_ColorAttachmentSampler));

						attachmentDescriptions[0].flags = 0;
						attachmentDescriptions[0].format = COLOR_BUFFER_FORMAT;
						attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
						attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
						attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					}

					// DEPTH ATTACHMENT

				}
			}

		}
	}

	void VulkanFramebuffer::AddResizeCallback(const std::function<void(Ref<HazelFramebuffer>)>& func)
	{
		m_ResizeCallbacks.push_back(func);
	}

}
