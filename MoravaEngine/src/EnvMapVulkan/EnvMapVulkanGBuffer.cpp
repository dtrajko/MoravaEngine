#include "EnvMapVulkanGBuffer.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Platform/Vulkan/VulkanPipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <array>
#include <vector>


static bool HasStencil(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM_S8_UINT;
}

void EnvMapVulkanGBuffer::Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
	H2M_CORE_ASSERT(framebuffer->GetSpecification().CopySource, "The G-buffer needs a scene framebuffer created with CopySource (its depth is stored)");
	m_DepthFormat = framebuffer->GetDepthVulkanFormat();
	CreateRenderPass();

	// Nearest: the normals, roughness and motion are read per pixel (SSAO, the debug views), never blended
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));

	Resize(sceneFramebuffer);
}

void EnvMapVulkanGBuffer::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (VkPipeline* pipeline : { &m_StaticPipeline, &m_SkinnedPipeline })
	{
		if (*pipeline)
		{
			vkDestroyPipeline(device, *pipeline, nullptr);
			*pipeline = VK_NULL_HANDLE;
		}
	}
	DestroyTargets();
	if (m_RenderPass)
	{
		vkDestroyRenderPass(device, m_RenderPass, nullptr);
		m_RenderPass = VK_NULL_HANDLE;
	}
	if (m_Sampler)
	{
		vkDestroySampler(device, m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}
}

void EnvMapVulkanGBuffer::CreateRenderPass()
{
	std::array<VkAttachmentDescription, 3> attachments = {};
	const VkFormat colorFormats[ColorAttachmentCount] = { NormalRoughnessFormat, MotionFormat };
	for (uint32_t i = 0; i < ColorAttachmentCount; i++)
	{
		attachments[i].format = colorFormats[i];
		attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[i].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	// The scene's depth: cleared and written here, kept for the scene pass (in the layout its render passes use)
	VkAttachmentDescription& depth = attachments[ColorAttachmentCount];
	depth.format = m_DepthFormat;
	depth.samples = VK_SAMPLE_COUNT_1_BIT;
	depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkAttachmentReference, ColorAttachmentCount> colorReferences = { {
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
		{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } } };
	VkAttachmentReference depthReference = { ColorAttachmentCount, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = ColorAttachmentCount;
	subpass.pColorAttachments = colorReferences.data();
	subpass.pDepthStencilAttachment = &depthReference;

	std::array<VkSubpassDependency, 2> dependencies = {};
	// In: the previous frame's readers of the images (shaders) and users of the depth (the scene's passes, the copies)
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
		VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// Out: the scene pass loads and tests the depth; shaders and compute passes read the normal and motion images
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
		VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t)attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	VK_CHECK_RESULT_H2M(vkCreateRenderPass(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), &renderPassInfo, nullptr, &m_RenderPass));
}

void EnvMapVulkanGBuffer::CreateAttachment(Attachment& attachment, VkFormat format)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { m_Width, m_Height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &attachment.Image));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, attachment.Image, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("GBuffer"));
	allocator.Allocate(requirements, &attachment.Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, attachment.Image, attachment.Memory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = attachment.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &attachment.View));
}

void EnvMapVulkanGBuffer::DestroyTargets()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (m_Framebuffer)
	{
		vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
		m_Framebuffer = VK_NULL_HANDLE;
	}
	if (m_DepthView)
	{
		vkDestroyImageView(device, m_DepthView, nullptr);
		m_DepthView = VK_NULL_HANDLE;
	}
	for (Attachment* attachment : { &m_NormalRoughness, &m_Motion })
	{
		if (attachment->View) vkDestroyImageView(device, attachment->View, nullptr);
		if (attachment->Image) vkDestroyImage(device, attachment->Image, nullptr);
		if (attachment->Memory) vkFreeMemory(device, attachment->Memory, nullptr);
		*attachment = Attachment();
	}
	m_SceneDepthImage = VK_NULL_HANDLE;
}

void EnvMapVulkanGBuffer::Resize(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	H2M::RefH2M<H2M::VulkanFramebufferH2M> framebuffer = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>();
	const uint32_t width = framebuffer->GetWidth(), height = framebuffer->GetHeight();
	VkImage sceneDepth = framebuffer->GetDepthVulkanImage();
	if (width == 0 || height == 0 || sceneDepth == VK_NULL_HANDLE ||
		(width == m_Width && height == m_Height && sceneDepth == m_SceneDepthImage && m_Framebuffer))
	{
		return;
	}
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	vkDeviceWaitIdle(device); // the old images may still be in use
	DestroyTargets();
	m_Width = width;
	m_Height = height;
	m_SceneDepthImage = sceneDepth;

	CreateAttachment(m_NormalRoughness, NormalRoughnessFormat);
	CreateAttachment(m_Motion, MotionFormat);

	// A view of the scene's depth image, like the scene framebuffer's own (an attachment view of a depth / stencil
	// format covers both aspects)
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_SceneDepthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = m_DepthFormat;
	viewInfo.subresourceRange = { (VkImageAspectFlags)(HasStencil(m_DepthFormat) ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT),
		0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_DepthView));

	std::array<VkImageView, 3> views = { m_NormalRoughness.View, m_Motion.View, m_DepthView };
	VkFramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = m_RenderPass;
	framebufferInfo.attachmentCount = (uint32_t)views.size();
	framebufferInfo.pAttachments = views.data();
	framebufferInfo.width = m_Width;
	framebufferInfo.height = m_Height;
	framebufferInfo.layers = 1;
	VK_CHECK_RESULT_H2M(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffer));
}

void EnvMapVulkanGBuffer::BeginPass(VkCommandBuffer commandBuffer)
{
	std::array<VkClearValue, 3> clearValues = {};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // no surface (the sky)
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } }; // no motion
	clearValues[2].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = m_RenderPass;
	beginInfo.framebuffer = m_Framebuffer;
	beginInfo.renderArea.extent = { m_Width, m_Height };
	beginInfo.clearValueCount = (uint32_t)clearValues.size();
	beginInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = { 0.0f, 0.0f, (float)m_Width, (float)m_Height, 0.0f, 1.0f };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	VkRect2D scissor = { { 0, 0 }, { m_Width, m_Height } };
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void EnvMapVulkanGBuffer::EndPass(VkCommandBuffer commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

static VkFormat VertexAttributeFormat(H2M::ShaderDataTypeH2M type)
{
	switch (type)
	{
		case H2M::ShaderDataTypeH2M::Float:  return VK_FORMAT_R32_SFLOAT;
		case H2M::ShaderDataTypeH2M::Float2: return VK_FORMAT_R32G32_SFLOAT;
		case H2M::ShaderDataTypeH2M::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
		case H2M::ShaderDataTypeH2M::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case H2M::ShaderDataTypeH2M::Int:    return VK_FORMAT_R32_SINT;
		case H2M::ShaderDataTypeH2M::Int2:   return VK_FORMAT_R32G32_SINT;
		case H2M::ShaderDataTypeH2M::Int3:   return VK_FORMAT_R32G32B32_SINT;
		case H2M::ShaderDataTypeH2M::Int4:   return VK_FORMAT_R32G32B32A32_SINT;
		default:                             return VK_FORMAT_UNDEFINED;
	}
}

void EnvMapVulkanGBuffer::CreatePipelines(H2M::RefH2M<H2M::PipelineH2M> staticMeshPipeline, H2M::RefH2M<H2M::PipelineH2M> skinnedMeshPipeline)
{
	m_StaticPipeline = CreatePipeline("GBufferPrepass_Static", staticMeshPipeline);
	m_SkinnedPipeline = CreatePipeline("GBufferPrepass_Anim", skinnedMeshPipeline);
}

VkPipeline EnvMapVulkanGBuffer::CreatePipeline(const std::string& shaderName, H2M::RefH2M<H2M::PipelineH2M> meshPipeline)
{
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get(shaderName).As<H2M::VulkanShaderH2M>();
	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();

	// The vertex layout of the PBR pipeline: the same vertex buffers are bound
	const H2M::VertexBufferLayoutH2M& vertexLayout = meshPipeline->GetSpecification().Layout;
	VkVertexInputBindingDescription binding = { 0, vertexLayout.GetStride(), VK_VERTEX_INPUT_RATE_VERTEX };
	std::vector<VkVertexInputAttributeDescription> attributes;
	for (const H2M::VertexBufferElementH2M& element : vertexLayout)
	{
		attributes.push_back({ (uint32_t)attributes.size(), 0, VertexAttributeFormat(element.Type), element.Offset });
	}
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;
	vertexInput.vertexAttributeDescriptionCount = (uint32_t)attributes.size();
	vertexInput.pVertexAttributeDescriptions = attributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// As in the PBR pipelines: no culling (VulkanPipelineH2M draws meshes two-sided), so the same surfaces win the depth test
	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	// The data is written as it is (no blending)
	std::array<VkPipelineColorBlendAttachmentState, ColorAttachmentCount> blendAttachments = {};
	for (VkPipelineColorBlendAttachmentState& attachment : blendAttachments)
	{
		attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = (uint32_t)blendAttachments.size();
	colorBlend.pAttachments = blendAttachments.data();

	std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = (uint32_t)stages.size();
	pipelineInfo.pStages = stages.data();
	pipelineInfo.pVertexInputState = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterization;
	pipelineInfo.pMultisampleState = &multisample;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlend;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = meshPipeline.As<H2M::VulkanPipelineH2M>()->GetVulkanPipelineLayout();
	pipelineInfo.renderPass = m_RenderPass;
	pipelineInfo.subpass = 0;

	VkPipeline pipeline = VK_NULL_HANDLE;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
	return pipeline;
}
