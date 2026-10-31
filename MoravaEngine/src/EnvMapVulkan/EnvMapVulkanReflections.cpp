#include "EnvMapVulkanReflections.h"

#include "EnvMapVulkanGBuffer.h"
#include "EnvMapVulkanRayTracing.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <cmath>
#include <vector>


static_assert(sizeof(EnvMapVulkanReflections::TracePush) == 112, "layout mismatch with the ReflectionTracePush block in ReflectionTrace.glsl");
static_assert(sizeof(EnvMapVulkanReflections::ResolvePush) == 32, "layout mismatch with the ReflectionResolvePush block in ReflectionResolve.glsl");

void EnvMapVulkanReflections::Create()
{
	// Nearest: every pass reads single texels and weighs them itself, and the mesh shaders read their own pixel
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.maxAnisotropy = 1.0f;
	m_Sampler = H2M::VulkanShaderH2M::GetSampler(samplerInfo); // shared: not destroyed here

	m_None.Create(Format, 1, 1, 1, 0.0f);

	// Without ray tracing there is nothing to trace with: the reflection image stays empty
	if (!H2M::VulkanContextH2M::GetCurrentDevice()->IsRayTracingSupported())
	{
		return;
	}
	// The trace only compiles for a device with ray tracing, so it is loaded here, not with the other shaders
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/ReflectionTrace.glsl");
	m_TracePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ReflectionTrace"));
	m_TraceFrameSet = m_TracePipeline->GetShader()->CreateDescriptorSets(0);
	m_TraceSet = m_TracePipeline->GetShader()->CreateDescriptorSets(1);
	m_ResolvePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("ReflectionResolve"));
	for (H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set : m_ResolveSets)
	{
		set = m_ResolvePipeline->GetShader()->CreateDescriptorSets();
	}
	m_TemporalFilter.Create("TemporalFilter_RGBA16F", Format);
}

void EnvMapVulkanReflections::Destroy()
{
	if (m_TracePipeline)
	{
		m_TemporalFilter.Destroy();
	}
	for (EnvMapVulkanComputeImage* image : { &m_Raw, &m_Depth, &m_Reflection, &m_None })
	{
		image->Destroy();
	}
	if (m_Sampler)
	{
		m_Sampler = VK_NULL_HANDLE;
	}
	m_TracePipeline = m_ResolvePipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_Width = m_Height = m_FullWidth = m_FullHeight = 0;
}

VkDescriptorImageInfo EnvMapVulkanReflections::GetReflectionInfo() const
{
	return m_Reflection.Handle ? m_Reflection.GetSampledInfo(m_Sampler) : GetNoneInfo();
}

void EnvMapVulkanReflections::Resize(const EnvMapVulkanGBuffer& gbuffer, bool halfResolution)
{
	m_Scale = halfResolution ? 2 : 1;
	if (!gbuffer.IsValid() || !m_TracePipeline)
	{
		return;
	}
	vkDeviceWaitIdle(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice()); // the old images may still be in use
	for (EnvMapVulkanComputeImage* image : { &m_Raw, &m_Depth, &m_Reflection })
	{
		image->Destroy();
	}
	m_FullWidth = gbuffer.GetWidth();
	m_FullHeight = gbuffer.GetHeight();
	// A half-size pixel stands for the full-size pixel at twice its coordinates: an odd size rounds up
	m_Width = (m_FullWidth + m_Scale - 1) / m_Scale;
	m_Height = (m_FullHeight + m_Scale - 1) / m_Scale;

	m_Raw.Create(Format, m_Width, m_Height, 1, 0.0f);
	m_Depth.Create(DepthFormat, m_Width, m_Height);
	m_Reflection.Create(Format, m_FullWidth, m_FullHeight, 1, 0.0f);
	m_IsEmpty = true;
	m_TemporalFilter.Resize(m_Width, m_Height);
	WriteDescriptors(gbuffer);
}

void EnvMapVulkanReflections::WriteDescriptors(const EnvMapVulkanGBuffer& gbuffer)
{
	const VkDescriptorImageInfo sceneDepth = gbuffer.GetDepthInfo();
	const VkDescriptorImageInfo normalRoughness = gbuffer.GetNormalRoughnessInfo();
	const VkDescriptorImageInfo raw = m_Raw.GetSampledInfo(m_Sampler);
	const VkDescriptorImageInfo depth = m_Depth.GetSampledInfo(m_Sampler);
	const VkDescriptorImageInfo rawTarget = m_Raw.GetStorageInfo();
	const VkDescriptorImageInfo depthTarget = m_Depth.GetStorageInfo();
	const VkDescriptorImageInfo reflectionTarget = m_Reflection.GetStorageInfo();
	const VkDescriptorImageInfo history[2] = { m_TemporalFilter.GetHistoryInfo(0), m_TemporalFilter.GetHistoryInfo(1) };

	std::vector<VkWriteDescriptorSet> writes;
	auto add = [&writes](H2M::RefH2M<H2M::VulkanComputePipelineH2M> pipeline, uint32_t setNumber, const H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet& set,
		const char* name, const VkDescriptorImageInfo* info)
	{
		VkWriteDescriptorSet write = *pipeline->GetShader()->GetDescriptorSet(name, setNumber);
		write.dstSet = set.DescriptorSets[0];
		write.descriptorCount = 1;
		write.pImageInfo = info;
		writes.push_back(write);
	};
	add(m_TracePipeline, 1, m_TraceSet, "u_SceneDepth", &sceneDepth);
	add(m_TracePipeline, 1, m_TraceSet, "u_NormalRoughness", &normalRoughness);
	add(m_TracePipeline, 1, m_TraceSet, "o_Reflection", &rawTarget);
	add(m_TracePipeline, 1, m_TraceSet, "o_Depth", &depthTarget);
	const VkDescriptorImageInfo* resolveSources[ResolveSetCount] = { &raw, &history[0], &history[1] };
	for (uint32_t i = 0; i < ResolveSetCount; i++)
	{
		add(m_ResolvePipeline, 0, m_ResolveSets[i], "u_Reflection", resolveSources[i]);
		add(m_ResolvePipeline, 0, m_ResolveSets[i], "u_Depth", &depth);
		add(m_ResolvePipeline, 0, m_ResolveSets[i], "u_SceneDepth", &sceneDepth);
		add(m_ResolvePipeline, 0, m_ResolveSets[i], "u_NormalRoughness", &normalRoughness);
		add(m_ResolvePipeline, 0, m_ResolveSets[i], "o_Reflection", &reflectionTarget);
	}
	vkUpdateDescriptorSets(H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice(), (uint32_t)writes.size(), writes.data(), 0, nullptr);

	m_TemporalFilter.SetInputs(raw, depth, gbuffer.GetMotionInfo());
}

void EnvMapVulkanReflections::Prepare(VkDescriptorSet frameDescriptorSet)
{
	if (m_TracePipeline)
	{
		EnvMapVulkanRayTracing::CopyFrameSet(frameDescriptorSet, m_TraceFrameSet.DescriptorSets[0]);
	}
}

void EnvMapVulkanReflections::Record(VkCommandBuffer commandBuffer, const EnvMapVulkanReflectionSettings& settings, EnvMapVulkanGBuffer& gbuffer,
	const glm::mat4& viewProjection, const glm::mat4& projection, const glm::vec3& cameraPosition, Stage stage,
	VkDescriptorSet rayTracingSet, bool sunShadows)
{
	m_Traced = false;
	if (!m_Reflection.Handle)
	{
		return;
	}
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

	if (!settings.Enabled || rayTracingSet == VK_NULL_HANDLE)
	{
		// The mesh shaders read the reflection image whatever the setting: empty is "the environment's reflection"
		if (!m_IsEmpty)
		{
			barrier(fragment, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
			const VkClearColorValue empty = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			const VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			vkCmdClearColorImage(commandBuffer, m_Reflection.Handle, VK_IMAGE_LAYOUT_GENERAL, &empty, 1, &range);
			barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);
			m_IsEmpty = true;
		}
		m_TemporalFilter.Reset();
		return;
	}
	m_IsEmpty = false;
	m_Traced = true;

	const bool temporal = settings.Temporal && stage == Stage::Final;
	m_Frame = temporal ? (m_Frame + 1) % 1024 : 0; // without the temporal filter the noise stands still: no flicker
	const float maxRoughness = glm::clamp(settings.MaxRoughness, EnvMapVulkanReflectionSettings::MinMaxRoughness, EnvMapVulkanReflectionSettings::MaxMaxRoughness);

	TracePush tracePush;
	tracePush.InverseViewProjection = glm::inverse(viewProjection);
	tracePush.CameraPosition = glm::vec4(cameraPosition, (float)m_Frame);
	// The angle a full-size pixel covers: the view's height at distance 1 is 2 / P[1][1]
	const float pixelAngle = 2.0f / (std::abs(projection[1][1]) * (float)std::max(m_FullHeight, 1u));
	tracePush.Params = glm::vec4(maxRoughness, (float)m_Scale, sunShadows ? 1.0f : 0.0f, pixelAngle);
	tracePush.Depth = glm::vec4(projection[2][2], projection[3][2], projection[2][3], 0.0f);
	ResolvePush resolvePush;
	resolvePush.Params = glm::vec4(maxRoughness, (float)m_Scale, settings.Denoise && stage == Stage::Final ? 1.0f : 0.0f, 0.0f);
	resolvePush.Depth = tracePush.Depth;

	// The depth buffer for the passes that sample it (the pass before left it as an attachment)
	gbuffer.MakeDepthReadable(commandBuffer, compute);
	// After the last frame's readers of these images
	barrier(compute | fragment, readWrite, compute, readWrite);

	// 1. The rays: the per-frame set's contents, the pass's own set, the ray tracing set
	{
		const VkPipelineLayout layout = m_TracePipeline->GetVulkanPipelineLayout();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_TracePipeline->GetVulkanPipeline());
		const VkDescriptorSet sets[2] = { m_TraceFrameSet.DescriptorSets[0], m_TraceSet.DescriptorSets[0] };
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, sets, 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 3, 1, &rayTracingSet, 0, nullptr);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(tracePush), &tracePush);
		vkCmdDispatch(commandBuffer, (m_Width + 7) / 8, (m_Height + 7) / 8, 1);
	}

	// 2. Averaged over the frames
	uint32_t resolveSource = ResolveRaw;
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, readWrite);
	if (temporal)
	{
		m_TemporalFilter.Record(commandBuffer);
		resolveSource = ResolveHistory0 + m_TemporalFilter.GetResultIndex();
		barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, compute, VK_ACCESS_SHADER_READ_BIT);
	}
	else
	{
		m_TemporalFilter.Reset();
	}

	// 3. To the viewport's size
	{
		const VkPipelineLayout layout = m_ResolvePipeline->GetVulkanPipelineLayout();
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ResolvePipeline->GetVulkanPipeline());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, m_ResolveSets[resolveSource].DescriptorSets.data(), 0, nullptr);
		vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(resolvePush), &resolvePush);
		vkCmdDispatch(commandBuffer, (m_FullWidth + 7) / 8, (m_FullHeight + 7) / 8, 1);
	}

	// The reflection image for the scene pass's mesh shaders, and the depth buffer back for its depth test
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);
	gbuffer.MakeDepthWritable(commandBuffer, compute);
}
