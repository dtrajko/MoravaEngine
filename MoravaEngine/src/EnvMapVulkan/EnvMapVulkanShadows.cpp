#include "EnvMapVulkanShadows.h"

#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>


void ComputeShadowCascades(const glm::vec3& cameraPosition, const glm::vec3& cameraForward, const std::array<glm::vec3, 4>& frustumCornerRays,
	float nearDistance, const EnvMapVulkanShadowSettings& settings, const glm::vec3& towardsLight,
	const glm::vec3& sceneBoundsMin, const glm::vec3& sceneBoundsMax, std::array<EnvMapVulkanShadowCascade, ShadowCascadeCount>& cascades)
{
	const float nearD = std::max(nearDistance, 0.1f);
	const float farD = std::max(settings.Distance, nearD + 1.0f);
	const float lambda = std::clamp(settings.SplitLambda, 0.0f, 1.0f);
	const float resolution = (float)std::max(settings.Resolution, 1u);

	// Split distances: the "practical split scheme" (Zhang et al. 2006), logarithmic and equal splits blended by lambda
	float splits[ShadowCascadeCount];
	for (uint32_t i = 0; i < ShadowCascadeCount; i++)
	{
		float p = (float)(i + 1) / ShadowCascadeCount;
		float logarithmic = nearD * std::pow(farD / nearD, p);
		float uniform = nearD + (farD - nearD) * p;
		splits[i] = lambda * logarithmic + (1.0f - lambda) * uniform;
	}

	const glm::vec3 light = glm::normalize(towardsLight);
	const glm::vec3 up = std::abs(light.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

	const bool hasScene = glm::all(glm::lessThanEqual(sceneBoundsMin, sceneBoundsMax));
	std::array<glm::vec3, 8> sceneCorners;
	for (int c = 0; c < 8; c++)
	{
		sceneCorners[c] = glm::vec3((c & 1) ? sceneBoundsMax.x : sceneBoundsMin.x, (c & 2) ? sceneBoundsMax.y : sceneBoundsMin.y,
			(c & 4) ? sceneBoundsMax.z : sceneBoundsMin.z);
	}

	float sliceStart = nearD;
	for (uint32_t i = 0; i < ShadowCascadeCount; i++)
	{
		const float sliceEnd = splits[i];

		// The slice's 8 corners: along each corner ray, at the view distances sliceStart and sliceEnd
		std::array<glm::vec3, 8> corners;
		glm::vec3 center(0.0f);
		for (int c = 0; c < 4; c++)
		{
			float cosAngle = std::max(glm::dot(frustumCornerRays[c], cameraForward), 1e-3f);
			corners[c] = cameraPosition + frustumCornerRays[c] * (sliceStart / cosAngle);
			corners[c + 4] = cameraPosition + frustumCornerRays[c] * (sliceEnd / cosAngle);
			center += corners[c] + corners[c + 4];
		}
		center /= 8.0f;

		// A sphere around the slice: its size doesn't depend on where the camera looks. Rounded up, so floating point
		// noise doesn't change the cascade's size (and the texel grid) from frame to frame.
		float radius = 0.0f;
		for (const glm::vec3& corner : corners)
		{
			radius = std::max(radius, glm::length(corner - center));
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		// The shadow camera sits back towards the sun, behind every caster (a caster outside the camera's view can
		// still cast into it), and sees the whole sphere
		float back = radius;
		if (hasScene)
		{
			for (const glm::vec3& corner : sceneCorners)
			{
				back = std::max(back, glm::dot(corner - center, light));
			}
		}
		back += 1.0f;

		glm::mat4 view = glm::lookAt(center + light * back, center, up);
		glm::mat4 projection = glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, back + radius);

		// Snap to whole texels: when the camera moves, the shadow map moves in steps of one texel, so the edges of the
		// shadows are sampled the same way every frame (no shimmering)
		glm::vec4 origin = projection * view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec2 originTexels = glm::vec2(origin) * (resolution * 0.5f);
		glm::vec2 offset = (glm::round(originTexels) - originTexels) * (2.0f / resolution);
		projection[3][0] += offset.x;
		projection[3][1] += offset.y;

		cascades[i].ViewProjection = projection * view;
		cascades[i].SplitDistance = sliceEnd;
		cascades[i].TexelWorldSize = 2.0f * radius / resolution;

		// The next slice starts a tenth of this one earlier: the blend zone between the two cascades is in both maps
		sliceStart = sliceEnd - 0.1f * (sliceEnd - sliceStart);
	}
}

float GetLocalShadowNearPlane(float range)
{
	return std::max(range * 0.005f, 0.01f);
}

glm::mat4 ComputeSpotShadowViewProjection(const glm::vec3& position, const glm::vec3& direction, float outerAngle, float range)
{
	const glm::vec3 forward = glm::normalize(direction);
	const glm::vec3 up = std::abs(forward.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAt(position, position + forward, up);
	glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(GetSpotShadowFieldOfView(outerAngle)), 1.0f, GetLocalShadowNearPlane(range),
		std::max(range, 0.02f));
	return projection * view;
}

float GetSpotShadowFieldOfView(float outerAngle)
{
	// The outer half-angle is at most 89 degrees (see EnvMapVulkanLightEnvironment::Pack)
	return std::min(2.0f * std::clamp(outerAngle, 0.1f, 89.0f) + 5.0f, 170.0f);
}

glm::vec2 GetPointShadowDepthParams(float range)
{
	const float n = GetLocalShadowNearPlane(range), f = std::max(range, 0.02f);
	return glm::vec2(f / (f - n), f * n / (f - n));
}

std::array<glm::mat4, 6> ComputePointShadowFaceViewProjections(const glm::vec3& position, float range)
{
	// Per face (layer order +X, -X, +Y, -Y, +Z, -Z): the major axis F, and the directions R and D along which the face's
	// s and t texture coordinates grow (Vulkan spec, Cube Map Face Selection: e.g. +X: sc = -rz, tc = -ry).
	// The view looks down F with R as its x axis and D as its y axis: then a perspective projection gives
	// ndc.x = dot(r, R) / dot(r, F) = 2s - 1 and ndc.y = dot(r, D) / dot(r, F) = 2t - 1, and Vulkan's viewport puts ndc
	// (-1, -1) at the top left texel (s, t = 0), as the lookup expects. (R, D, -F) is right-handed for every face.
	struct Face { glm::vec3 F, R, D; };
	static const Face faces[6] = {
		{ { 1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } }, // +X: sc = -rz, tc = -ry
		{ {-1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } }, // -X: sc = +rz, tc = -ry
		{ { 0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } }, // +Y: sc = +rx, tc = +rz
		{ { 0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } }, // -Y: sc = +rx, tc = -rz
		{ { 0.0f,  0.0f,  1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // +Z: sc = +rx, tc = -ry
		{ { 0.0f,  0.0f, -1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // -Z: sc = -rx, tc = -ry
	};
	const glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(90.0f), 1.0f, GetLocalShadowNearPlane(range), std::max(range, 0.02f));
	std::array<glm::mat4, 6> matrices;
	for (int i = 0; i < 6; i++)
	{
		const Face& face = faces[i];
		// Rows of the view rotation: R, D, -F (camera space x, y, z); then the translation to the light's position
		glm::mat4 view(1.0f);
		view[0] = glm::vec4(face.R.x, face.D.x, -face.F.x, 0.0f);
		view[1] = glm::vec4(face.R.y, face.D.y, -face.F.y, 0.0f);
		view[2] = glm::vec4(face.R.z, face.D.z, -face.F.z, 0.0f);
		view[3] = glm::vec4(-glm::dot(face.R, position), -glm::dot(face.D, position), glm::dot(face.F, position), 1.0f);
		matrices[i] = projection * view;
	}
	return matrices;
}

float PointShadowDepth(const glm::vec3& offset, float range)
{
	// The largest coordinate is the distance along the face's major axis (the view depth); perspectiveRH_ZO maps a view
	// depth z to far / (far - near) - far * near / ((far - near) * z)
	const glm::vec2 params = GetPointShadowDepthParams(range);
	const float z = std::max(std::abs(offset.x), std::max(std::abs(offset.y), std::abs(offset.z)));
	return params.x - params.y / z;
}

void EnvMapVulkanShadowMap::Create(uint32_t resolution, uint32_t layerCount, bool cubeArray)
{
	Destroy();

	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	m_Resolution = resolution;
	m_LayerCount = layerCount;
	m_CubeArray = cubeArray;
	m_Generation++;
	m_LayerViews.assign(layerCount, VK_NULL_HANDLE);
	m_DisplayViews.assign(layerCount, VK_NULL_HANDLE);
	m_Framebuffers.assign(layerCount, VK_NULL_HANDLE);

	// One depth image, a layer per shadow map (6 per cube)
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.flags = cubeArray ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = Format;
	imageInfo.extent = { resolution, resolution, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = layerCount;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	H2M::VulkanAllocatorH2M allocator(std::string("ShadowMap"));
	m_Allocation = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Image);

	// Views: all layers (sampled by the PBR shaders) and one per layer (rendered into, and shown in the UI)
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_Image;
	viewInfo.viewType = cubeArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	viewInfo.format = Format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, layerCount };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_ArrayView));
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_FlatArrayView));
	for (uint32_t i = 0; i < layerCount; i++)
	{
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, i, 1 };
		VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_LayerViews[i]));
		// For the UI: a depth image samples as (depth, 0, 0, 1); repeating red in green and blue shows it as gray
		viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE };
		VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_DisplayViews[i]));
		viewInfo.components = {};
	}

	// Depth-only render pass: cleared, written, then left ready for the PBR shaders to sample
	VkAttachmentDescription attachment = {};
	attachment.format = Format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // the previous contents are never needed
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pDepthStencilAttachment = &depthReference;

	std::array<VkSubpassDependency, 2> dependencies = {};
	// The previous frame's PBR shaders must be done reading the map before it is cleared and rewritten
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	// The depth writes must be finished (and visible) before the PBR shaders sample the map
	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = (uint32_t)dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	VK_CHECK_RESULT_H2M(vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass));

	for (uint32_t i = 0; i < layerCount; i++)
	{
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &m_LayerViews[i];
		framebufferInfo.width = resolution;
		framebufferInfo.height = resolution;
		framebufferInfo.layers = 1;
		VK_CHECK_RESULT_H2M(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
	}

	// Comparison sampler (sampler2DArrayShadow): returns how much of the 2x2 texels around the lookup is lit, so even a
	// single lookup has a slightly soft edge. Outside the map (border): lit.
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.compareEnable = VK_TRUE;
	samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // lit when the fragment's depth <= the depth in the map
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_CompareSampler));

	// Plain sampler for showing the depth layers in the UI
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_DisplaySampler));

	// Start in the sampling layout, so the PBR shaders can bind the map before the first shadow pass (or with shadows off)
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_Image;
	barrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, layerCount };
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	vulkanDevice->FlushCommandBuffer(commandBuffer);
}

void EnvMapVulkanShadowMap::Destroy()
{
	if (!IsValid())
	{
		return;
	}
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	vkDestroySampler(device, m_DisplaySampler, nullptr);
	vkDestroySampler(device, m_CompareSampler, nullptr);
	for (VkFramebuffer& framebuffer : m_Framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
		framebuffer = VK_NULL_HANDLE;
	}
	vkDestroyRenderPass(device, m_RenderPass, nullptr);
	for (VkImageView& view : m_LayerViews)
	{
		vkDestroyImageView(device, view, nullptr);
		view = VK_NULL_HANDLE;
	}
	for (VkImageView& view : m_DisplayViews)
	{
		vkDestroyImageView(device, view, nullptr);
		view = VK_NULL_HANDLE;
	}
	vkDestroyImageView(device, m_ArrayView, nullptr);
	vkDestroyImageView(device, m_FlatArrayView, nullptr);
	m_FlatArrayView = VK_NULL_HANDLE;
	H2M::VulkanAllocatorH2M allocator(std::string("ShadowMap"));
	allocator.DestroyImage(m_Image, m_Allocation);

	m_DisplaySampler = m_CompareSampler = VK_NULL_HANDLE;
	m_RenderPass = VK_NULL_HANDLE;
	m_ArrayView = VK_NULL_HANDLE;
	m_Image = VK_NULL_HANDLE;
	m_Allocation = nullptr;
	m_Resolution = 0;
	m_LayerCount = 0;
	m_LayerViews.clear();
	m_DisplayViews.clear();
	m_Framebuffers.clear();
}

static VkFormat ToVulkanFormat(H2M::ShaderDataTypeH2M type)
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

void EnvMapVulkanShadowPipeline::Create(H2M::RefH2M<H2M::VulkanShaderH2M> shader, const H2M::VertexBufferLayoutH2M& vertexLayout,
	const std::vector<uint32_t>& usedLocations, VkRenderPass renderPass)
{
	Destroy();
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Layout: the shader's descriptor sets (the bone buffer of ShadowDepth_Anim.glsl) and push constants
	std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
	std::vector<VkPushConstantRange> pushConstantRanges;
	for (const auto& range : shader->GetPushConstantRanges())
	{
		pushConstantRanges.push_back({ (VkShaderStageFlags)range.ShaderStage, range.Offset, range.Size });
	}
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
	layoutInfo.pSetLayouts = setLayouts.data();
	layoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
	layoutInfo.pPushConstantRanges = pushConstantRanges.data();
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &Layout));

	// Vertex input: the mesh's full vertex (stride), with only the attributes the shader reads
	VkVertexInputBindingDescription binding = { 0, vertexLayout.GetStride(), VK_VERTEX_INPUT_RATE_VERTEX };
	std::vector<VkVertexInputAttributeDescription> attributes;
	uint32_t location = 0;
	for (const auto& element : vertexLayout)
	{
		if (std::find(usedLocations.begin(), usedLocations.end(), location) != usedLocations.end())
		{
			attributes.push_back({ location, 0, ToVulkanFormat(element.Type), element.Offset });
		}
		location++;
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

	VkPipelineRasterizationStateCreateInfo rasterization = {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization.cullMode = VK_CULL_MODE_NONE;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.depthBiasEnable = VK_TRUE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisample = {};
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineColorBlendStateCreateInfo colorBlend = {};
	colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; // no color attachments

	std::array<VkDynamicState, 3> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = (uint32_t)dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	const std::vector<VkPipelineShaderStageCreateInfo>& stages = shader->GetPipelineShaderStageCreateInfos();
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
	pipelineInfo.layout = Layout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	VK_CHECK_RESULT_H2M(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &Pipeline));
}

void EnvMapVulkanShadowPipeline::Destroy()
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (Pipeline)
	{
		vkDestroyPipeline(device, Pipeline, nullptr);
		Pipeline = VK_NULL_HANDLE;
	}
	if (Layout)
	{
		vkDestroyPipelineLayout(device, Layout, nullptr);
		Layout = VK_NULL_HANDLE;
	}
}

void EnvMapVulkanShadowMapViewer::Create(H2M::RefH2M<H2M::VulkanShaderH2M> shader)
{
	Destroy();
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	m_Shader = shader;

	// The output: written by the compute shader, sampled by ImGui
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageInfo.extent = { Width, Height, 1 };
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	H2M::VulkanAllocatorH2M allocator(std::string("ShadowMapViewer"));
	m_Allocation = allocator.AllocateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY, m_Image);

	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = imageInfo.format;
	viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VK_CHECK_RESULT_H2M(vkCreateImageView(device, &viewInfo, nullptr, &m_View));

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	VK_CHECK_RESULT_H2M(vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler));

	// Compute pipeline, from the shader's reflection
	std::vector<VkDescriptorSetLayout> setLayouts = shader->GetAllDescriptorSetLayouts();
	std::vector<VkPushConstantRange> pushConstantRanges;
	for (const auto& range : shader->GetPushConstantRanges())
	{
		pushConstantRanges.push_back({ (VkShaderStageFlags)range.ShaderStage, range.Offset, range.Size });
	}
	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = (uint32_t)setLayouts.size();
	layoutInfo.pSetLayouts = setLayouts.data();
	layoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
	layoutInfo.pPushConstantRanges = pushConstantRanges.data();
	VK_CHECK_RESULT_H2M(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_Layout));

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = shader->GetPipelineShaderStageCreateInfos()[0];
	pipelineInfo.layout = m_Layout;
	VK_CHECK_RESULT_H2M(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

	m_DescriptorSet = shader->CreateDescriptorSets(0);
	m_BoundMap = nullptr;

	// Cleared to the viewer's background, and in the layout ImGui samples
	H2M::RefH2M<H2M::VulkanDeviceH2M> vulkanDevice = H2M::VulkanContextH2M::GetCurrentDevice();
	VkCommandBuffer commandBuffer = vulkanDevice->GetCommandBuffer(true);
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_Image;
	barrier.subresourceRange = viewInfo.subresourceRange;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	VkClearColorValue clearColor = { { 0.1f, 0.1f, 0.1f, 1.0f } };
	vkCmdClearColorImage(commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &viewInfo.subresourceRange);
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	vulkanDevice->FlushCommandBuffer(commandBuffer);
}

void EnvMapVulkanShadowMapViewer::Record(VkCommandBuffer commandBuffer, const EnvMapVulkanShadowMap& shadowMap, bool cube, uint32_t baseLayer,
	float nearPlane, float farPlane)
{
	if (!IsValid() || !shadowMap.IsValid() || m_DescriptorSet.DescriptorSets.empty())
	{
		return;
	}
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Point the descriptor set at this shadow map (when it changed: another map, or the map was recreated). The previous
	// frame is finished (one frame in flight), and the set is bound below, after the update.
	if (m_BoundMap != &shadowMap || m_BoundGeneration != shadowMap.GetGeneration())
	{
		VkDescriptorImageInfo input = { shadowMap.GetDisplaySampler(), shadowMap.GetFlatArrayView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		VkDescriptorImageInfo output = { VK_NULL_HANDLE, m_View, VK_IMAGE_LAYOUT_GENERAL };
		std::array<VkWriteDescriptorSet, 2> writes = { *m_Shader->GetDescriptorSet("u_ShadowMap"), *m_Shader->GetDescriptorSet("o_View") };
		writes[0].dstSet = writes[1].dstSet = m_DescriptorSet.DescriptorSets[0];
		writes[0].descriptorCount = writes[1].descriptorCount = 1;
		writes[0].pImageInfo = &input;
		writes[1].pImageInfo = &output;
		vkUpdateDescriptorSets(device, (uint32_t)writes.size(), writes.data(), 0, nullptr);
		m_BoundMap = &shadowMap;
		m_BoundGeneration = shadowMap.GetGeneration();
	}

	// The shadow passes' depth writes before the compute shader reads them; the output from sampled (last frame's UI) to written
	VkMemoryBarrier depthBarrier = {};
	depthBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	depthBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	VkImageMemoryBarrier outputBarrier = {};
	outputBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	outputBarrier.srcQueueFamilyIndex = outputBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	outputBarrier.image = m_Image;
	outputBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	outputBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	outputBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	outputBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0, 1, &depthBarrier, 0, nullptr, 1, &outputBarrier);

	struct Settings
	{
		int32_t Mode;
		int32_t BaseLayer;
		float Near;
		float Far;
	} settings = { cube ? 1 : 0, (int32_t)baseLayer, nearPlane, farPlane };
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Layout, 0, 1, m_DescriptorSet.DescriptorSets.data(), 0, nullptr);
	vkCmdPushConstants(commandBuffer, m_Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(Settings), &settings);
	vkCmdDispatch(commandBuffer, (Width + 15) / 16, (Height + 15) / 16, 1);

	// Written before ImGui samples it
	outputBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	outputBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	outputBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	outputBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &outputBarrier);
}

void EnvMapVulkanShadowMapViewer::Destroy()
{
	if (!IsValid())
	{
		return;
	}
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	if (m_DescriptorSet.Pool)
	{
		vkDestroyDescriptorPool(device, m_DescriptorSet.Pool, nullptr);
	}
	m_DescriptorSet = H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet();
	vkDestroyPipeline(device, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(device, m_Layout, nullptr);
	vkDestroySampler(device, m_Sampler, nullptr);
	vkDestroyImageView(device, m_View, nullptr);
	H2M::VulkanAllocatorH2M allocator(std::string("ShadowMapViewer"));
	allocator.DestroyImage(m_Image, m_Allocation);
	m_Pipeline = VK_NULL_HANDLE;
	m_Layout = VK_NULL_HANDLE;
	m_Sampler = VK_NULL_HANDLE;
	m_View = VK_NULL_HANDLE;
	m_Image = VK_NULL_HANDLE;
	m_Allocation = nullptr;
	m_BoundMap = nullptr;
}
