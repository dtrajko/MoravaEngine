#include "EnvMapVulkanProbes.h"

#include "EnvMapVulkanProfiler.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanFramebufferH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <array>
#include <vector>


static_assert(sizeof(EnvMapVulkanProbes::ProbeVolumeUB) == 80, "std140 layout mismatch with the ProbeVolume block in Include/FrameSet.glslh");

// The push constants of ProbeSpheres.glsl's vertex stage
struct ProbeSpheresPush
{
	glm::vec4 OriginRadius;
	glm::vec4 Spacing;
	glm::ivec4 Counts;
	glm::vec4 CameraRight;
	glm::vec4 CameraUp;
};

void EnvMapVulkanProbes::Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Linear for the two atlases read per direction (the borders make it work up to a tile's edge), nearest for the
	// probes' own values
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_LinearSampler));
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_NearestSampler));

	m_FillPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ProbeEnvFill"));
	m_FillDescriptorSet = m_FillPipeline->GetShader()->CreateDescriptorSets();

	CreateSpherePipeline(sceneFramebuffer);

	// The mesh shaders' set 0 always has the atlases, also in a scene without a probe volume: the smallest ones then
	CreateAtlases(glm::ivec3(EnvMapVulkanProbeVolumeSettings::MinCount));
}

void EnvMapVulkanProbes::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	DestroyAtlases();
	if (m_SpherePipeline) vkDestroyPipeline(device, m_SpherePipeline, nullptr);
	if (m_SpherePipelineLayout) vkDestroyPipelineLayout(device, m_SpherePipelineLayout, nullptr);
	if (m_LinearSampler) vkDestroySampler(device, m_LinearSampler, nullptr);
	if (m_NearestSampler) vkDestroySampler(device, m_NearestSampler, nullptr);
	m_SpherePipeline = VK_NULL_HANDLE;
	m_SpherePipelineLayout = VK_NULL_HANDLE;
	m_LinearSampler = VK_NULL_HANDLE;
	m_NearestSampler = VK_NULL_HANDLE;
	m_FillPipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_FillDescriptorSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
}

void EnvMapVulkanProbes::CreateAtlas(Atlas& atlas, VkFormat format, uint32_t width, uint32_t height)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent = { width, height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; // written by compute passes, sampled by the mesh shaders
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK_RESULT_H2M(vkCreateImage(device, &imageInfo, nullptr, &atlas.Image));

	VkMemoryRequirements requirements;
	vkGetImageMemoryRequirements(device, atlas.Image, &requirements);
	H2M::VulkanAllocatorH2M allocator(std::string("Probes"));
	allocator.Allocate(requirements, &atlas.Memory);
	VK_CHECK_RESULT_H2M(vkBindImageMemory(device, atlas.Image, atlas.Memory, 0));

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = atlas.Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &atlas.View));
	atlas.Width = width;
	atlas.Height = height;
}

void EnvMapVulkanProbes::CreateAtlases(const glm::ivec3& counts)
{
	m_Counts = counts;
	const uint32_t tilesX = (uint32_t)(counts.x * counts.y), tilesY = (uint32_t)counts.z;
	CreateAtlas(m_Irradiance, IrradianceFormat, tilesX * (IrradianceTexels + 2), tilesY * (IrradianceTexels + 2));
	CreateAtlas(m_Visibility, VisibilityFormat, tilesX * (VisibilityTexels + 2), tilesY * (VisibilityTexels + 2));
	CreateAtlas(m_ProbeData, ProbeDataFormat, tilesX, tilesY);

	// Into the layout they keep (see the class comment)
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	std::array<VkImageMemoryBarrier, 3> barriers = {};
	const VkImage images[3] = { m_Irradiance.Image, m_Visibility.Image, m_ProbeData.Image };
	for (size_t i = 0; i < barriers.size(); i++)
	{
		barriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[i].image = images[i];
		barriers[i].subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	}
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, (uint32_t)barriers.size(), barriers.data());
	vulkanDevice->FlushCommandBuffer(commandBuffer);

	m_Filled = false;
}

void EnvMapVulkanProbes::DestroyAtlases()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	for (Atlas* atlas : { &m_Irradiance, &m_Visibility, &m_ProbeData })
	{
		if (atlas->View) vkDestroyImageView(device, atlas->View, nullptr);
		if (atlas->Image) vkDestroyImage(device, atlas->Image, nullptr);
		if (atlas->Memory) vkFreeMemory(device, atlas->Memory, nullptr);
		*atlas = Atlas();
	}
	m_Counts = glm::ivec3(0);
}

bool EnvMapVulkanProbes::Update(const EnvMapVulkanProbeVolumeSettings& settings, const VkDescriptorImageInfo& environmentIrradiance,
	float environmentRotation, uint32_t environmentVersion)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	m_Settings = settings;

	// Without a probe volume the (smallest) atlases stay as they are: nothing reads them
	bool replaced = false;
	const glm::ivec3 counts = settings.Exists ? settings.GetCounts() : m_Counts;
	if (counts != m_Counts)
	{
		vkDeviceWaitIdle(device); // the last frame may still sample the old atlases
		DestroyAtlases();
		CreateAtlases(counts);
		replaced = true;
	}

	const glm::vec3 spacing = settings.GetSpacing();
	const bool current = m_Filled && m_FilledRotation == environmentRotation && m_FilledEnvironmentVersion == environmentVersion &&
		(!settings.Exists || m_FilledSpacing == spacing);
	if (!current)
	{
		vkDeviceWaitIdle(device); // the last frame may still sample the atlases this writes
		FillFromEnvironment(environmentIrradiance, environmentRotation);
		m_Filled = true;
		m_FilledSpacing = spacing;
		m_FilledRotation = environmentRotation;
		m_FilledEnvironmentVersion = environmentVersion;
	}
	return replaced;
}

void EnvMapVulkanProbes::FillFromEnvironment(const VkDescriptorImageInfo& environmentIrradiance, float environmentRotation)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = m_FillPipeline->GetShader();

	// Storage image descriptors: the view and the layout (no sampler)
	const VkDescriptorImageInfo storageInfos[3] = {
		{ VK_NULL_HANDLE, m_Irradiance.View, VK_IMAGE_LAYOUT_GENERAL },
		{ VK_NULL_HANDLE, m_Visibility.View, VK_IMAGE_LAYOUT_GENERAL },
		{ VK_NULL_HANDLE, m_ProbeData.View, VK_IMAGE_LAYOUT_GENERAL } };
	const char* storageNames[3] = { "o_Irradiance", "o_Visibility", "o_ProbeData" };
	std::array<VkWriteDescriptorSet, 4> writes;
	writes[0] = *shader->GetDescriptorSet("u_EnvIrradianceTex");
	writes[0].dstSet = m_FillDescriptorSet.DescriptorSets[0];
	writes[0].descriptorCount = 1;
	writes[0].pImageInfo = &environmentIrradiance;
	for (uint32_t i = 0; i < 3; i++)
	{
		writes[1 + i] = *shader->GetDescriptorSet(storageNames[i]);
		writes[1 + i].dstSet = m_FillDescriptorSet.DescriptorSets[0];
		writes[1 + i].descriptorCount = 1;
		writes[1 + i].pImageInfo = &storageInfos[i];
	}
	vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);

	// Every probe sees farther than any point its cell's corners can ask about (a cell's diagonal), so no probe is
	// counted as hidden
	const float seenDistance = 1.5f * glm::length(m_Settings.GetSpacing());
	const glm::vec4 params(environmentRotation, seenDistance, IrradianceGamma, 0.0f);

	// One thread per texel of the largest atlas, in groups of 8 x 8 (see ProbeEnvFill.glsl)
	m_FillPipeline->Begin();
	m_FillPipeline->SetPushConstants(&params, sizeof(params));
	m_FillPipeline->Dispatch(m_FillDescriptorSet.DescriptorSets[0], (m_Visibility.Width + 7) / 8, (m_Visibility.Height + 7) / 8, 1);
	m_FillPipeline->End();
	vkQueueWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue());
}

EnvMapVulkanProbes::ProbeVolumeUB EnvMapVulkanProbes::GetUniforms() const
{
	ProbeVolumeUB ub;
	ub.Origin = glm::vec4(m_Settings.GetOrigin(), m_Settings.LightsScene() ? 1.0f : 0.0f);
	ub.Spacing = glm::vec4(m_Settings.GetSpacing(), IrradianceGamma);
	ub.Counts = glm::ivec4(m_Counts, 0);
	ub.Biases = glm::vec4(m_Settings.NormalBias, m_Settings.ViewBias, 0.0f, 0.0f);
	ub.AtlasSizes = glm::vec4((float)m_Irradiance.Width, (float)m_Irradiance.Height, (float)m_Visibility.Width, (float)m_Visibility.Height);
	return ub;
}

void EnvMapVulkanProbes::CreateSpherePipeline(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	H2M::RefH2M<H2M::VulkanShaderH2M> shader = H2M::RendererH2M::GetShaderLibrary()->Get("ProbeSpheres").As<H2M::VulkanShaderH2M>();
	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();

	// Set 0 only (the shader declares it as the PBR shaders do, so the per-frame set fits), and the vertex stage's push constants
	VkDescriptorSetLayout setLayout = shader->GetDescriptorSetLayout(H2M::VulkanShaderH2M::FrameDescriptorSet);
	VkPushConstantRange pushRange = { VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)sizeof(ProbeSpheresPush) };
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &setLayout;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges = &pushRange;
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_SpherePipelineLayout));

	// No vertex buffers: the vertex stage makes the squares from the vertex and instance index
	VkPipelineVertexInputStateCreateInfo vertexInput = {};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Solid balls: tested against the scene's depth and written into it (the fragment stage gives the ball's depth)
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineColorBlendAttachmentState blendAttachment = {};
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlend.attachmentCount = 1;
	colorBlend.pAttachments = &blendAttachment;

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
	pipelineInfo.layout = m_SpherePipelineLayout;
	// The scene framebuffer's render pass (its continue render pass, used after the water's scene copy, is compatible)
	pipelineInfo.renderPass = sceneFramebuffer.As<H2M::VulkanFramebufferH2M>()->GetRenderPass();
	pipelineInfo.subpass = 0;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_SpherePipeline));
}

void EnvMapVulkanProbes::RecordSpheres(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const glm::mat4& cameraView)
{
	if (!ShowsSpheres())
	{
		return;
	}
	// The camera's right and up directions in the world: the first two rows of the view matrix's rotation
	ProbeSpheresPush push;
	push.OriginRadius = glm::vec4(m_Settings.GetOrigin(), m_Settings.ProbeRadius);
	push.Spacing = glm::vec4(m_Settings.GetSpacing(), 0.0f);
	push.Counts = glm::ivec4(m_Counts, 0);
	push.CameraRight = glm::vec4(cameraView[0][0], cameraView[1][0], cameraView[2][0], 0.0f);
	push.CameraUp = glm::vec4(cameraView[0][1], cameraView[1][1], cameraView[2][1], 0.0f);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SpherePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_SpherePipelineLayout, H2M::VulkanShaderH2M::FrameDescriptorSet, 1,
		&frameDescriptorSet, 0, nullptr);
	vkCmdPushConstants(commandBuffer, m_SpherePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
	vkCmdDraw(commandBuffer, 6, GetProbeCount(), 0, 0);
	EnvMapVulkanProfiler::CountDraw(2 * GetProbeCount());
}
