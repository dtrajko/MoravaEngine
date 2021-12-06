/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanFramebufferH2M.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"

#include "Core/Application.h"


namespace H2M
{

	VulkanFramebufferH2M::VulkanFramebufferH2M(const FramebufferSpecificationH2M& spec)
		: m_Specification(spec)
	{
		if (spec.Width == 0 || spec.Height == 0)
		{
			m_Width = Application::Get()->GetWindow()->GetWidth();
			m_Height = Application::Get()->GetWindow()->GetHeight();
		}
		else
		{
			m_Width = spec.Width;
			m_Height = spec.Height;
		}

		// Create attachment descriptors immediately
		m_Attachments.emplace_back(Image2D_H2M::Create(ImageFormatH2M::RGBA32F, m_Width, m_Height));
		m_Attachments.emplace_back(Image2D_H2M::Create(ImageFormatH2M::Depth, m_Width, m_Height));

		Resize((uint32_t)(m_Width * spec.Scale), (uint32_t)(m_Height * spec.Scale), true);
	}

	void VulkanFramebufferH2M::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		m_Width = width;
		m_Height = height;

		if (!m_Specification.SwapChainTarget)
		{
			// RefH2M<VulkanFramebuffer> instance = this;
			// RendererH2M::Submit([instance, width, height]() mutable
			// {
			// });
			{
				auto device = VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

				if (m_Framebuffer) {
					vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
					m_Framebuffer = nullptr;
				}

				VulkanAllocatorH2M allocator(std::string("Framebuffer"));

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

					VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &m_ColorAttachment.image));

					VkMemoryRequirements memoryRequirements;
					vkGetImageMemoryRequirements(device, m_ColorAttachment.image, &memoryRequirements);

					allocator.Allocate(memoryRequirements, &m_ColorAttachment.mem);

					VK_CHECK_RESULT(vkBindImageMemory(device, m_ColorAttachment.image, m_ColorAttachment.mem, 0));

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
					colorImageViewCreateInfo.image = m_ColorAttachment.image;
					VK_CHECK_RESULT(vkCreateImageView(device, &colorImageViewCreateInfo, nullptr, &m_ColorAttachment.view));

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
					VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_ColorAttachmentSampler));

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
				{
					VkFormat depthFormat = VulkanContextH2M::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();

					VkImageCreateInfo imageCreateInfo = {};
					imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
					imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
					imageCreateInfo.format = depthFormat;
					imageCreateInfo.extent.width = width;
					imageCreateInfo.extent.height = height;
					imageCreateInfo.extent.depth = 1;
					imageCreateInfo.mipLevels = 1;
					imageCreateInfo.arrayLayers = 1;
					imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
					imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
					// We will sample directly from the depth attachment
					imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

					VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &m_DepthAttachment.image));
					VkMemoryRequirements memoryRequirements;
					vkGetImageMemoryRequirements(device, m_DepthAttachment.image, &memoryRequirements);
					allocator.Allocate(memoryRequirements, &m_DepthAttachment.mem);

					VK_CHECK_RESULT(vkBindImageMemory(device, m_DepthAttachment.image, m_DepthAttachment.mem, 0));

					VkImageViewCreateInfo depthStencilImageViewCreateInfo = {};
					depthStencilImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					depthStencilImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					depthStencilImageViewCreateInfo.format = depthFormat;
					depthStencilImageViewCreateInfo.flags = 0;
					depthStencilImageViewCreateInfo.subresourceRange = {};
					depthStencilImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
					depthStencilImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
					depthStencilImageViewCreateInfo.subresourceRange.levelCount = 1;
					depthStencilImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
					depthStencilImageViewCreateInfo.subresourceRange.layerCount = 1;
					depthStencilImageViewCreateInfo.image = m_DepthAttachment.image;
					VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilImageViewCreateInfo, nullptr, &m_DepthAttachment.view));

					// Depth attachment
					attachmentDescriptions[1].flags = 0;
					attachmentDescriptions[1].format = depthFormat;
					attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
					attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
					attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
					attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
					attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}

				VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
				VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

				VkSubpassDescription subpassDescription = {};
				subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpassDescription.colorAttachmentCount = 1;
				subpassDescription.pColorAttachments = &colorReference;
				subpassDescription.pDepthStencilAttachment = &depthReference;

				// Use subpass dependencies for layout transitions
				std::array<VkSubpassDependency, 2> dependencies;

				dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
				dependencies[0].dstSubpass = 0;
				dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

				dependencies[1].srcSubpass = 0;
				dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
				dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

				// Create the actual renderpass
				VkRenderPassCreateInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
				renderPassInfo.pAttachments = attachmentDescriptions.data();
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpassDescription;
				renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
				renderPassInfo.pDependencies = dependencies.data();

				VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass));

				VkImageView attachments[2];
				attachments[0] = m_ColorAttachment.view;
				attachments[1] = m_DepthAttachment.view;

				VkFramebufferCreateInfo framebufferCreateInfo = {};
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCreateInfo.renderPass = m_RenderPass;
				framebufferCreateInfo.attachmentCount = 2;
				framebufferCreateInfo.pAttachments = attachments;
				framebufferCreateInfo.width = width;
				framebufferCreateInfo.height = height;
				framebufferCreateInfo.layers = 1;

				VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_Framebuffer));

				// Fill a descriptor for later use in a descriptor set
				m_DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				m_DescriptorImageInfo.imageView = m_ColorAttachment.view;
				m_DescriptorImageInfo.sampler = m_ColorAttachmentSampler;
			}
		}
		else
		{
			VulkanSwapChainH2M& swapChain = Application::Get()->GetWindow()->GetSwapChain();
			m_RenderPass = swapChain.GetRenderPass();
		}

		for (auto& callback : m_ResizeCallbacks)
		{
			callback(this);
		}
	}

	void VulkanFramebufferH2M::AddResizeCallback(const std::function<void(RefH2M<FramebufferH2M>)>& func)
	{
		m_ResizeCallbacks.push_back(func);
	}
}
