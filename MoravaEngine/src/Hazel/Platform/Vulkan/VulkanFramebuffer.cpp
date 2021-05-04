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
		if (spec.Width == 0)
		{
			m_Width = Application::Get()->GetWindow()->GetWidth();
			m_Height = Application::Get()->GetWindow()->GetHeight();
		}
		else
		{
			m_Width = spec.Width;
			m_Height = spec.Height;
		}

		HZ_CORE_ASSERT(spec.Attachments.Attachments.size());
		for (auto format : m_Specification.Attachments.Attachments)
		{
			if (!Utils::IsDepthFormat(format.Format))
				m_Attachments.emplace_back(HazelImage2D::Create(HazelImageFormat::RGBA32F, m_Width, m_Height));
			else
				m_DepthAttachment = HazelImage2D::Create(format.Format, m_Width, m_Height);
		}

		Resize(m_Width * (uint32_t)spec.Scale, m_Height * (uint32_t)spec.Scale, true);
	}

	VulkanFramebuffer::~VulkanFramebuffer()
	{
	}

	void VulkanFramebuffer::Resize(uint32_t width, uint32_t height, bool forceRecreate)
	{
		if (!forceRecreate && (m_Width == width && m_Height == height))
			return;

		m_Width = width;
		m_Height = height;
		if (!m_Specification.SwapChainTarget)
		{
			Ref<VulkanFramebuffer> instance = this;
			HazelRenderer::Submit([instance, width, height]() mutable
				{
					auto device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

					if (instance->m_Framebuffer)
					{
						vkDestroyFramebuffer(device, instance->m_Framebuffer, nullptr);
						for (auto image : instance->m_Attachments)
							image->Release();

						if (instance->m_DepthAttachment)
							instance->m_DepthAttachment->Release();
						//instance->m_Attachments.clear();
					}

					VulkanAllocator allocator(device, "Framebuffer");

					std::vector<VkAttachmentDescription> attachmentDescriptions;
					attachmentDescriptions.reserve(instance->m_Attachments.size());

					std::vector<VkAttachmentReference> colorAttachmentReferences(instance->m_Attachments.size());
					VkAttachmentReference depthAttachmentReference;

					uint32_t attachmentCount = (uint32_t)instance->m_Attachments.size() + (instance->m_DepthAttachment ? 1 : 0);
					instance->m_ClearValues.resize(attachmentCount);

					// Color attachments
					uint32_t attachmentIndex = 0;
					for (auto image : instance->m_Attachments)
					{
						const VkFormat COLOR_BUFFER_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT; // TODO: support more formats;

						Ref<VulkanImage2D> colorAttachment = image.As<VulkanImage2D>();
						auto& info = colorAttachment->GetImageInfo();

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
						VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &info.Image));

						VkMemoryRequirements memReqs;
						vkGetImageMemoryRequirements(device, info.Image, &memReqs);

						allocator.Allocate(memReqs, &info.Memory);

						VK_CHECK_RESULT(vkBindImageMemory(device, info.Image, info.Memory, 0));

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
						colorImageViewCreateInfo.image = info.Image;
						VK_CHECK_RESULT(vkCreateImageView(device, &colorImageViewCreateInfo, nullptr, &info.ImageView));

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
						samplerCreateInfo.maxLod = 1.0f;
						samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
						VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &info.Sampler));

						VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
						attachmentDescription.flags = 0;
						attachmentDescription.format = COLOR_BUFFER_FORMAT;
						attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
						attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
						attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

						colorAttachment->UpdateDescriptor();

						const auto& clearColor = instance->m_Specification.ClearColor;
						instance->m_ClearValues[attachmentIndex].color = { {clearColor.r, clearColor.g, clearColor.b, clearColor.a} };
						colorAttachmentReferences[attachmentIndex] = { attachmentIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
						attachmentIndex++;
					}

					if (instance->m_DepthAttachment)
					{
						VkFormat depthFormat = instance->m_DepthAttachment->GetFormat() == HazelImageFormat::DEPTH32F ? VK_FORMAT_D32_SFLOAT : VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetDepthFormat();

						auto& info = instance->m_DepthAttachment.As<VulkanImage2D>()->GetImageInfo();

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
						// TODO: we only need VK_IMAGE_USAGE_SAMPLED_BIT if we're planning to sample the depth attachment
						imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

						VK_CHECK_RESULT(vkCreateImage(device, &imageCreateInfo, nullptr, &info.Image));
						VkMemoryRequirements memoryRequirements;
						vkGetImageMemoryRequirements(device, info.Image, &memoryRequirements);
						allocator.Allocate(memoryRequirements, &info.Memory);

						VK_CHECK_RESULT(vkBindImageMemory(device, info.Image, info.Memory, 0));

						VkImageViewCreateInfo depthStencilImageViewCreateInfo = {};
						depthStencilImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
						depthStencilImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
						depthStencilImageViewCreateInfo.format = depthFormat;
						depthStencilImageViewCreateInfo.flags = 0;
						depthStencilImageViewCreateInfo.subresourceRange = {};
						depthStencilImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
						if (instance->m_DepthAttachment->GetFormat() == HazelImageFormat::DEPTH24STENCIL8)
							depthStencilImageViewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
						depthStencilImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
						depthStencilImageViewCreateInfo.subresourceRange.levelCount = 1;
						depthStencilImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
						depthStencilImageViewCreateInfo.subresourceRange.layerCount = 1;
						depthStencilImageViewCreateInfo.image = info.Image;
						VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilImageViewCreateInfo, nullptr, &info.ImageView));

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
						samplerCreateInfo.maxLod = 1.0f;
						samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
						VK_CHECK_RESULT(vkCreateSampler(device, &samplerCreateInfo, nullptr, &info.Sampler));

						VkAttachmentDescription& attachmentDescription = attachmentDescriptions.emplace_back();
						attachmentDescription.flags = 0;
						attachmentDescription.format = depthFormat;
						attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
						attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
						attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // TODO: if sampling, needs to be store (otherwise DONT_CARE is fine)
						attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
						attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
						attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // TODO: if not sampling
						attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // TODO: if sampling

						Ref<VulkanImage2D> image = instance->m_DepthAttachment.As<VulkanImage2D>();
						image->UpdateDescriptor();

						depthAttachmentReference = { attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

						instance->m_ClearValues[attachmentIndex].depthStencil = { 1.0f, 0 };
					}

					VkSubpassDescription subpassDescription = {};
					subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
					subpassDescription.colorAttachmentCount = (uint32_t)colorAttachmentReferences.size();
					subpassDescription.pColorAttachments = colorAttachmentReferences.data();
					if (instance->m_DepthAttachment)
						subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;

#if 1
					// TODO: do we need these?
					// Use subpass dependencies for layout transitions
					std::array<VkSubpassDependency, 2> dependencies;

#if 0
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
#endif

					dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
					dependencies[0].dstSubpass = 0;
					dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

					dependencies[1].srcSubpass = 0;
					dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
					dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
					dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
					dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
					dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
#endif

					// Create the actual renderpass
					VkRenderPassCreateInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
					renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
					renderPassInfo.pAttachments = attachmentDescriptions.data();
					renderPassInfo.subpassCount = 1;
					renderPassInfo.pSubpasses = &subpassDescription;
					renderPassInfo.dependencyCount = 0; ;// static_cast<uint32_t>(dependencies.size());
					renderPassInfo.pDependencies = nullptr;// dependencies.data();
					renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
					renderPassInfo.pDependencies = dependencies.data();

					VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &instance->m_RenderPass));

					std::vector<VkImageView> attachments(instance->m_Attachments.size());
					for (uint32_t i = 0; i < instance->m_Attachments.size(); i++)
					{
						Ref<VulkanImage2D> image = instance->m_Attachments[i].As<VulkanImage2D>();
						attachments[i] = image->GetImageInfo().ImageView;
					}

					if (instance->m_DepthAttachment)
					{
						Ref<VulkanImage2D> image = instance->m_DepthAttachment.As<VulkanImage2D>();
						attachments.emplace_back(image->GetImageInfo().ImageView);
					}

					VkFramebufferCreateInfo framebufferCreateInfo = {};
					framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferCreateInfo.renderPass = instance->m_RenderPass;
					framebufferCreateInfo.attachmentCount = (uint32_t)attachments.size();
					framebufferCreateInfo.pAttachments = attachments.data();
					framebufferCreateInfo.width = width;
					framebufferCreateInfo.height = height;
					framebufferCreateInfo.layers = 1;

					VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &instance->m_Framebuffer));
				});
		}
		else
		{
			VulkanSwapChain& swapChain = VulkanContext::Get()->GetSwapChain();
			m_RenderPass = swapChain.GetRenderPass();
		}

		for (auto& callback : m_ResizeCallbacks)
			callback(this);
	}

	void VulkanFramebuffer::AddResizeCallback(const std::function<void(Ref<HazelFramebuffer>)>& func)
	{
		m_ResizeCallbacks.push_back(func);
	}

}
