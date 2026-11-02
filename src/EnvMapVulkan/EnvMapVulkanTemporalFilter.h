#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>


/**
 * An image a compute pass writes and a later pass samples (SceneEnvMapVulkan: the ambient occlusion's images, the
 * temporal filter's history). It stays in VK_IMAGE_LAYOUT_GENERAL from Create on, so the passes are kept in order by
 * memory barriers alone, and is cleared to one value in all channels and levels.
 */
struct EnvMapVulkanComputeImage
{
	VkImage Handle = VK_NULL_HANDLE;
	VkDeviceMemory Memory = VK_NULL_HANDLE;
	VkImageView View = VK_NULL_HANDLE;   // all levels: for sampling
	std::vector<VkImageView> LevelViews; // one level each: for writing
	uint32_t Width = 0, Height = 0;

	// Waits for nothing: the caller makes sure the old image (if any) is no longer in use before Destroy
	void Create(VkFormat format, uint32_t width, uint32_t height, uint32_t levels = 1, float clearValue = 1.0f);
	void Destroy();

	VkDescriptorImageInfo GetSampledInfo(VkSampler sampler) const { return { sampler, View, VK_IMAGE_LAYOUT_GENERAL }; }
	VkDescriptorImageInfo GetStorageInfo(uint32_t level = 0) const { return { VK_NULL_HANDLE, LevelViews[level], VK_IMAGE_LAYOUT_GENERAL }; }
};

/**
 * The temporal filter (SceneEnvMapVulkan, Phase F of docs/rendering/SSAO_GI_RayTracing_Guide.html): averages a noisy
 * image over the frames. Made for the ambient occlusion and meant for every later effect that has a few samples per
 * pixel and frame (ray-traced occlusion and reflections): one filter object per effect.
 *
 * Each frame the caller's "current" image is mixed with the history, the filter's own result of the previous frame,
 * read where each pixel's surface point was then (the G-buffer's motion). The shader (Include/TemporalFilter.glslh)
 * drops the history where the depth says it belongs to another surface, limits it to the current frame's range around
 * the pixel, and counts the frames a pixel has, so a new pixel settles in a few frames.
 *
 * Owns: two history images of the caller's format and two of their depth and frame count (RG16F); a frame reads one
 * pair and writes the other, then they swap. One compute dispatch per frame, recorded into the caller's command buffer.
 * The result of a frame is GetResultInfo: the pair written last (a different image every frame, so a reader keeps a
 * descriptor set for each: GetHistoryInfo(0 / 1), picked by GetResultIndex).
 */
class EnvMapVulkanTemporalFilter
{
public:
	struct Push
	{
		float MaxFrames;
		float DepthTolerance;
		float NoHistory;
		float RangeMargin;
	};

	// shaderName: a compute shader that includes Include/TemporalFilter.glslh with the format ("TemporalFilter_R8")
	void Create(const std::string& shaderName, VkFormat format);
	void Destroy();
	// New history images of this size (the old ones must no longer be in use), without history. Then SetInputs.
	void Resize(uint32_t width, uint32_t height);
	// The image to filter and its linear depth (both of the history's size), and the G-buffer's motion (any size)
	void SetInputs(const VkDescriptorImageInfo& current, const VkDescriptorImageInfo& currentDepth, const VkDescriptorImageInfo& motion);
	// The next Record starts without history (the image it filters wasn't made in the last frame)
	void Reset() { m_HasHistory = false; }
	// After the pass that wrote the current image (the caller's barrier). historyWeight: the part of the result that is
	// history once a pixel has settled (0.9: ten frames).
	void Record(VkCommandBuffer commandBuffer, float historyWeight = 0.9f, float depthTolerance = 0.1f);

	uint32_t GetResultIndex() const { return m_Result; }
	VkDescriptorImageInfo GetHistoryInfo(uint32_t index) const { return m_History[index].GetSampledInfo(m_Sampler); }
	VkDescriptorImageInfo GetResultInfo() const { return GetHistoryInfo(m_Result); }

private:
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_Pipeline;
	// [i]: reads pair 1 - i, writes pair i
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_Sets[2];
	EnvMapVulkanComputeImage m_History[2];
	EnvMapVulkanComputeImage m_HistoryInfo[2];
	VkSampler m_Sampler = VK_NULL_HANDLE; // nearest: the shader weighs the texels itself
	VkFormat m_Format = VK_FORMAT_UNDEFINED;
	uint32_t m_Width = 0, m_Height = 0;
	uint32_t m_Result = 0;    // the pair the last Record wrote
	bool m_HasHistory = false;
};
