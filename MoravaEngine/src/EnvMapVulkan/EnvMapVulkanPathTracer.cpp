#include "EnvMapVulkanPathTracer.h"

#include "EnvMapVulkanRayTracing.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include <algorithm>
#include <cmath>


static_assert(sizeof(EnvMapVulkanPathTracer::Push) == 96, "layout mismatch with the PathTracePush block in PathTrace.glsl");

// FNV-1a over bytes, continued from hash
static uint64_t HashBytes(uint64_t hash, const void* data, size_t size)
{
	const unsigned char* bytes = static_cast<const unsigned char*>(data);
	for (size_t i = 0; i < size; i++)
	{
		hash = (hash ^ bytes[i]) * 1099511628211ull;
	}
	return hash;
}

void EnvMapVulkanPathTracer::Create()
{
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

	// Without ray tracing there is nothing to trace with: the image stays a black texel (Resize)
	if (!H2M::VulkanContextH2M::GetCurrentDevice()->IsRayTracingSupported())
	{
		return;
	}
	// The shader only compiles for a device with ray tracing, so it is loaded here, not with the other shaders
	H2M::RendererH2M::GetShaderLibrary()->Load("Resources/Shaders/PathTrace.glsl");
	m_Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>::Create(H2M::RendererH2M::GetShaderLibrary()->Get("PathTrace"));
	m_FrameSet = m_Pipeline->GetShader()->CreateDescriptorSets(0);
	m_ImageSet = m_Pipeline->GetShader()->CreateDescriptorSets(1);
}

void EnvMapVulkanPathTracer::Destroy()
{
	m_Image.Destroy();
	if (m_Sampler)
	{
		m_Sampler = VK_NULL_HANDLE;
	}
	m_Pipeline = H2M::RefH2M<H2M::VulkanComputePipelineH2M>();
	m_Samples = 0;
}

void EnvMapVulkanPathTracer::Resize(uint32_t width, uint32_t height)
{
	VkDevice device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
	vkDeviceWaitIdle(device); // the old image may still be in use
	m_Image.Destroy();
	m_Samples = 0;
	if (!m_Pipeline)
	{
		m_Image.Create(Format, 1, 1, 1, 0.0f);
		return;
	}
	m_Image.Create(Format, std::max(width, 1u), std::max(height, 1u), 1, 0.0f);

	const VkDescriptorImageInfo target = m_Image.GetStorageInfo();
	VkWriteDescriptorSet write = *m_Pipeline->GetShader()->GetDescriptorSet("u_Accumulation", 1);
	write.dstSet = m_ImageSet.DescriptorSets[0];
	write.descriptorCount = 1;
	write.pImageInfo = &target;
	vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void EnvMapVulkanPathTracer::Prepare(VkDescriptorSet frameDescriptorSet)
{
	if (m_Pipeline)
	{
		EnvMapVulkanRayTracing::CopyFrameSet(frameDescriptorSet, m_FrameSet.DescriptorSets[0]);
	}
}

void EnvMapVulkanPathTracer::Record(VkCommandBuffer commandBuffer, const EnvMapVulkanPathTracerSettings& settings, const glm::mat4& viewProjection,
	const glm::mat4& projection, uint64_t sceneVersion, float skyboxLod, VkDescriptorSet rayTracingSet, bool sunShadows)
{
	if (!m_Pipeline || rayTracingSet == VK_NULL_HANDLE)
	{
		return;
	}
	const int maxBounces = glm::clamp(settings.MaxBounces, 0, EnvMapVulkanPathTracerSettings::MaxMaxBounces);
	const float maxBrightness = std::max(settings.MaxBrightness, 0.0f);

	// Everything a path depends on: when any of it changed, the samples so far are of another image
	uint64_t version = HashBytes(14695981039346656037ull, &viewProjection, sizeof(viewProjection));
	version = HashBytes(version, &sceneVersion, sizeof(sceneVersion));
	version = HashBytes(version, &maxBounces, sizeof(maxBounces));
	version = HashBytes(version, &maxBrightness, sizeof(maxBrightness));
	version = HashBytes(version, &skyboxLod, sizeof(skyboxLod));
	version = HashBytes(version, &sunShadows, sizeof(sunShadows));
	if (version != m_Version)
	{
		m_Version = version;
		m_Samples = 0;
	}

	uint32_t count = (uint32_t)glm::clamp(settings.SamplesPerFrame, 1, EnvMapVulkanPathTracerSettings::MaxSamplesPerFrame);
	if (settings.MaxSamples > 0)
	{
		const uint32_t maxSamples = (uint32_t)settings.MaxSamples;
		count = m_Samples >= maxSamples ? 0 : std::min(count, maxSamples - m_Samples);
	}
	if (count == 0)
	{
		return; // finished: the image stays as it is
	}

	Push push;
	push.InverseViewProjection = glm::inverse(viewProjection);
	push.Counts = glm::ivec4((int)m_Samples, (int)count, maxBounces, 0);
	// The angle a pixel covers: the view's height at distance 1 is 2 / P[1][1]
	const float pixelAngle = 2.0f / (std::abs(projection[1][1]) * (float)std::max(m_Image.Height, 1u));
	push.Params = glm::vec4(sunShadows ? 1.0f : 0.0f, maxBrightness, skyboxLod, pixelAngle);

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

	// After the last frame's pass, which wrote the image, and its composite, which sampled it
	barrier(compute | fragment, readWrite, compute, readWrite);
	const VkPipelineLayout layout = m_Pipeline->GetVulkanPipelineLayout();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline->GetVulkanPipeline());
	const VkDescriptorSet sets[2] = { m_FrameSet.DescriptorSets[0], m_ImageSet.DescriptorSets[0] };
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 2, sets, 0, nullptr);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 3, 1, &rayTracingSet, 0, nullptr);
	vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
	vkCmdDispatch(commandBuffer, (m_Image.Width + 7) / 8, (m_Image.Height + 7) / 8, 1);
	barrier(compute, VK_ACCESS_SHADER_WRITE_BIT, fragment, VK_ACCESS_SHADER_READ_BIT);

	m_Samples += count;
}
