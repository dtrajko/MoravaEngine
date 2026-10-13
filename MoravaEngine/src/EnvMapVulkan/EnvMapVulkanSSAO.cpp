#include "EnvMapVulkanSSAO.h"

#include "EnvMapVulkanGBuffer.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <vector>


static_assert(sizeof(EnvMapVulkanSSAO::Push) == 128, "layout mismatch with the SSAOPush block in Include/SSAO.glslh");

void EnvMapVulkanSSAO::Pass::Create(const char* shaderName)
{
	Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get(shaderName));
}

void EnvMapVulkanSSAO::Create()
{
	// Nearest, with the depth image's smaller levels: every pass reads single texels and weighs them itself
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
	VK_CHECK_RESULT_H2M(vkCreateSampler(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), &samplerInfo, nullptr, &m_Sampler));

	m_White.Create(AOFormat, 1, 1);

	m_DepthPass.Create("SSAODepth");
	m_DepthMipPass.Create("SSAODepthMip");
	m_GTAOPass.Create("SSAOGTAO");
	m_DenoisePass.Create("SSAODenoise");
	m_UpsamplePass.Create("SSAOUpsample");
	m_DepthSet = m_DepthPass.Pipeline->GetShader()->CreateDescriptorSets();
	for (H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set : m_DepthMipSets)
	{
		set = m_DepthMipPass.Pipeline->GetShader()->CreateDescriptorSets();
	}
	m_GTAOSet = m_GTAOPass.Pipeline->GetShader()->CreateDescriptorSets();
	m_DenoiseSet = m_DenoisePass.Pipeline->GetShader()->CreateDescriptorSets();
	for (H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set : m_UpsampleSets)
	{
		set = m_UpsamplePass.Pipeline->GetShader()->CreateDescriptorSets();
	}
	m_TemporalFilter.Create("TemporalFilter_R8", AOFormat);

	// The ray-traced occlusion: a shader that only compiles for a device with ray tracing, so it is loaded here
	if (H2M::VulkanContextH2M::GetCurrentDevice()->IsRayTracingSupported())
	{
		H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/SSAORayTraced.glsl");
		m_RayTracedPass.Create("SSAORayTraced");
		m_RayTracedSet = m_RayTracedPass.Pipeline->GetShader()->CreateDescriptorSets(0);
	}
}

void EnvMapVulkanSSAO::Destroy()
{
	m_TemporalFilter.Destroy();
	for (EnvMapVulkanComputeImage* image : { &m_Depth, &m_Raw, &m_Denoised, &m_AO, &m_White })
	{
		image->Destroy();
	}
	if (m_Sampler)
	{
		vkDestroySampler(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), m_Sampler, nullptr);
		m_Sampler = VK_NULL_HANDLE;
	}
	for (Pass* pass : { &m_DepthPass, &m_DepthMipPass, &m_GTAOPass, &m_DenoisePass, &m_UpsamplePass, &m_RayTracedPass })
	{
		pass->Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	}
	m_Width = m_Height = m_FullWidth = m_FullHeight = 0;
}

VkDescriptorImageInfo EnvMapVulkanSSAO::GetAOInfo() const
{
	return m_AO.Handle ? m_AO.GetSampledInfo(m_Sampler) : GetWhiteInfo();
}

void EnvMapVulkanSSAO::Resize(const EnvMapVulkanGBuffer& gbuffer, bool halfResolution)
{
	if (!gbuffer.IsValid())
	{
		return;
	}
	m_Scale = halfResolution ? 2 : 1;
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // the old images may still be in use
	for (EnvMapVulkanComputeImage* image : { &m_Depth, &m_Raw, &m_Denoised, &m_AO })
	{
		image->Destroy();
	}
	m_FullWidth = gbuffer.GetWidth();
	m_FullHeight = gbuffer.GetHeight();
	// A half-size pixel stands for the full-size pixel at twice its coordinates: an odd size rounds up
	m_Width = (m_FullWidth + m_Scale - 1) / m_Scale;
	m_Height = (m_FullHeight + m_Scale - 1) / m_Scale;
	m_DepthLevels = 1;
	while (m_DepthLevels < MaxDepthLevels && (std::max(m_Width, m_Height) >> m_DepthLevels) > 0)
	{
		m_DepthLevels++;
	}

	m_Depth.Create(DepthFormat, m_Width, m_Height, m_DepthLevels);
	m_Raw.Create(AOFormat, m_Width, m_Height);
	m_Denoised.Create(AOFormat, m_Width, m_Height);
	m_AO.Create(AOFormat, m_FullWidth, m_FullHeight);
	m_AOIsWhite = true;
	m_TemporalFilter.Resize(m_Width, m_Height);
	WriteDescriptors(gbuffer);
}

void EnvMapVulkanSSAO::WriteDescriptors(const EnvMapVulkanGBuffer& gbuffer)
{
	const VkDescriptorImageInfo sceneDepth = gbuffer.GetDepthInfo();
	const VkDescriptorImageInfo normalRoughness = gbuffer.GetNormalRoughnessInfo();
	const VkDescriptorImageInfo depth = m_Depth.GetSampledInfo(m_Sampler);
	const VkDescriptorImageInfo raw = m_Raw.GetSampledInfo(m_Sampler);
	const VkDescriptorImageInfo denoised = m_Denoised.GetSampledInfo(m_Sampler);
	const VkDescriptorImageInfo rawTarget = m_Raw.GetStorageInfo();
	const VkDescriptorImageInfo denoisedTarget = m_Denoised.GetStorageInfo();
	const VkDescriptorImageInfo aoTarget = m_AO.GetStorageInfo();
	const VkDescriptorImageInfo history[2] = { m_TemporalFilter.GetHistoryInfo(0), m_TemporalFilter.GetHistoryInfo(1) };
	std::vector<VkDescriptorImageInfo> depthLevels(m_DepthLevels);
	for (uint32_t level = 0; level < m_DepthLevels; level++)
	{
		depthLevels[level] = m_Depth.GetStorageInfo(level);
	}

	std::vector<VkWriteDescriptorSet> writes;
	auto add = [&writes](Pass& pass, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, const char* name, const VkDescriptorImageInfo* info)
	{
		VkWriteDescriptorSet write = *pass.Pipeline->GetShader()->GetDescriptorSet(name);
		write.dstSet = set.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = info;
		writes.push_back(write);
	};
	add(m_DepthPass, m_DepthSet, "u_SceneDepth", &sceneDepth);
	add(m_DepthPass, m_DepthSet, "o_Depth", &depthLevels[0]);
	for (uint32_t level = 1; level < MaxDepthLevels; level++)
	{
		// The sets of levels this size doesn't have are never used: they get the last level, so they are complete
		add(m_DepthMipPass, m_DepthMipSets[level - 1], "u_Depth", &depth);
		add(m_DepthMipPass, m_DepthMipSets[level - 1], "o_Depth", &depthLevels[std::min(level, m_DepthLevels - 1)]);
	}
	add(m_GTAOPass, m_GTAOSet, "u_Depth", &depth);
	add(m_GTAOPass, m_GTAOSet, "u_NormalRoughness", &normalRoughness);
	add(m_GTAOPass, m_GTAOSet, "o_AO", &rawTarget);
	if (m_RayTracedPass.Pipeline)
	{
		add(m_RayTracedPass, m_RayTracedSet, "u_Depth", &depth);
		add(m_RayTracedPass, m_RayTracedSet, "u_NormalRoughness", &normalRoughness);
		add(m_RayTracedPass, m_RayTracedSet, "o_AO", &rawTarget);
	}
	add(m_DenoisePass, m_DenoiseSet, "u_AO", &raw);
	add(m_DenoisePass, m_DenoiseSet, "u_Depth", &depth);
	add(m_DenoisePass, m_DenoiseSet, "u_NormalRoughness", &normalRoughness);
	add(m_DenoisePass, m_DenoiseSet, "o_AO", &denoisedTarget);
	const VkDescriptorImageInfo* upsampleSources[UpsampleSetCount] = { &raw, &denoised, &history[0], &history[1] };
	for (uint32_t i = 0; i < UpsampleSetCount; i++)
	{
		add(m_UpsamplePass, m_UpsampleSets[i], "u_AO", upsampleSources[i]);
		add(m_UpsamplePass, m_UpsampleSets[i], "u_Depth", &depth);
		add(m_UpsamplePass, m_UpsampleSets[i], "u_SceneDepth", &sceneDepth);
		add(m_UpsamplePass, m_UpsampleSets[i], "o_AO", &aoTarget);
	}
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

	m_TemporalFilter.SetInputs(denoised, depth, gbuffer.GetMotionInfo());
}

void EnvMapVulkanSSAO::Record(VkCommandBuffer commandBuffer, const EnvMapVulkanSSAOSettings& settings, EnvMapVulkanGBuffer& gbuffer,
	const glm::mat4& view, const glm::mat4& projection, Stage stage, VkDescriptorSet rayTracingSet)
{
	if (!m_AO.Handle)
	{
		return;
	}
	m_RayTraced = settings.Enabled && settings.RayTraced && rayTracingSet != VK_NULL_HANDLE && m_RayTracedPass.Pipeline;
	auto barrier = [commandBuffer](VkPipelineStageFlags sourceStages, VkAccessFlags sourceAccess, VkPipelineStageFlags destinationStages, VkAccessFlags destinationAccess)
	{
		VkMemoryBarrier memoryBarrier = {};
		memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		memoryBarrier.srcAccessMask = sourceAccess;
		memoryBarrier.dstAccessMask = destinationAccess;
		vkCmdPipelineBarrier(commandBuffer, sourceStages, destinationStages, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
	};
	const VkAccessFlags readWrite = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	const VkPipelineStageFlags compute = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	const VkPipelineStageFlags fragment = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	if (!settings.Enabled)
	{
		// The mesh shaders read the AO image whatever the setting: white is "no occlusion"
		if (!m_AOIsWhite)
		{
			barrier(fragment, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
			const VkClearColorValue white = { { 1.0f, 1.0f, 1.0f, 1.0f } };
			const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdClearColorImage(commandBuffer, m_AO.Handle, VK_IMAGE_LAYOUT_GENERAL, &white, 1, &range);
			barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);
			m_AOIsWhite = true;
		}
		m_TemporalFilter.Reset();
		return;
	}
	m_AOIsWhite = false;

	const bool temporal = settings.Temporal && stage == Stage::Final;
	m_Frame = temporal ? (m_Frame + 1) % 64 : 0; // without the temporal filter the noise stands still: no flicker

	Push push;
	push.View = view;
	push.Projection = glm::vec4(projection[0][0] * 0.5f, projection[1][1] * 0.5f, projection[2][3], (float)m_Scale);
	push.Depth = glm::vec4(projection[2][2], projection[3][2], 0.0f, 0.0f);
	push.Params = glm::vec4(std::max(settings.Radius, 0.01f), settings.Power, settings.Falloff, (float)m_Frame);
	push.Counts = glm::ivec4(glm::clamp(settings.Slices, 1, EnvMapVulkanSSAOSettings::MaxSlices),
		glm::clamp(settings.Steps, 1, EnvMapVulkanSSAOSettings::MaxSteps), (int)m_DepthLevels, 0);

	auto dispatch = [commandBuffer, &push](const Pass& pass, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set, uint32_t width, uint32_t height)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.Pipeline->GetVulkanPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pass.Pipeline->GetVulkanPipelineLayout(), 0, 1, set.DescriptorSets.data(), 0, nullptr);
		vkCmdPushConstants(commandBuffer, pass.Pipeline->GetVulkanPipelineLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
		vkCmdDispatch(commandBuffer, (width + 7) / 8, (height + 7) / 8, 1);
	};

	// The depth buffer for the two passes that sample it (the prepass left it as an attachment)
	gbuffer.MakeDepthReadable(commandBuffer, compute);
	// After the last frame's readers of these images
	barrier(compute | fragment, readWrite, compute, readWrite);

	dispatch(m_DepthPass, m_DepthSet, m_Width, m_Height);
	if (m_RayTraced)
	{
		// Rays against the scene's geometry: the depth's smaller levels aren't needed (they are GTAO's far lookups)
		barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
		push.Counts.x = glm::clamp(settings.Rays, 1, EnvMapVulkanSSAOSettings::MaxRays);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RayTracedPass.Pipeline->GetVulkanPipelineLayout(), 3, 1, &rayTracingSet, 0, nullptr);
		dispatch(m_RayTracedPass, m_RayTracedSet, m_Width, m_Height);
	}
	else
	{
		for (uint32_t level = 1; level < m_DepthLevels; level++)
		{
			barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
			push.Counts.w = (int)level - 1; // the level read
			dispatch(m_DepthMipPass, m_DepthMipSets[level - 1], std::max(m_Width >> level, 1u), std::max(m_Height >> level, 1u));
		}

		barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
		dispatch(m_GTAOPass, m_GTAOSet, m_Width, m_Height);
	}

	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
	push.Counts.w = settings.Denoise ? 1 : 0;
	dispatch(m_DenoisePass, m_DenoiseSet, m_Width, m_Height);

	uint32_t upsampleSource = stage == Stage::Raw ? UpsampleRaw : UpsampleDenoised;
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, readWrite);
	if (temporal)
	{
		m_TemporalFilter.Record(commandBuffer);
		upsampleSource = UpsampleHistory0 + m_TemporalFilter.GetResultIndex();
		barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
	}
	else
	{
		m_TemporalFilter.Reset();
	}

	dispatch(m_UpsamplePass, m_UpsampleSets[upsampleSource], m_FullWidth, m_FullHeight);

	// The AO image for the scene pass's mesh shaders, and the depth buffer back for its depth test
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);
	gbuffer.MakeDepthWritable(commandBuffer, compute);
}
