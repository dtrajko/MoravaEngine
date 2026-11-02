#pragma once

#include "EnvMapVulkanTemporalFilter.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>


// The Path Traced view's settings (Environment panel > Debug View)
struct EnvMapVulkanPathTracerSettings
{
	int MaxBounces = 8;         // the times a path is scattered by a surface before it ends (0: the lights' light only)
	int SamplesPerFrame = 1;    // paths per pixel and frame
	int MaxSamples = 4096;      // the image is finished with this many per pixel (0: it never is)
	float MaxBrightness = 50.0f; // what one path can add to a pixel after its first bounce (0: no limit). A rare path that
	                            // finds a small, very bright spot would leave a white dot for thousands of samples.

	static constexpr int MaxMaxBounces = 32;
	static constexpr int MaxSamplesPerFrame = 16;
};

/**
 * The path tracer (SceneEnvMapVulkan, Phase H.4 of docs/rendering/SSAO_GI_RayTracing_Guide.html): the scene drawn by rays
 * alone, each pixel's light followed through all its bounces. Far too slow and noisy for the editor's view, but it is
 * what the probes, the ambient occlusion and the reflections approximate: a reference to compare them with (the Path
 * Traced debug view).
 *
 * One compute pass (PathTrace.glsl) adds a few random paths per pixel to the image, which holds the average of all paths
 * so far (linear HDR, the viewport's size; the viewport composite shows it in the scene's place: GetImageInfo). The
 * average starts over when anything a path depends on changed: the camera, the scene (the caller's version number), a
 * setting, the image's size. Standing still, the image gets cleaner with every frame, until Max Samples.
 *
 * Needs a GPU with ray tracing. Without one, the image is a black texel and Record does nothing.
 *
 * The pass lights what a path hits with the per-frame set's contents (lights, environment): a compute pipeline can't use
 * the mesh shaders' set 0, so it has its own, filled in Prepare.
 *
 * Owns its image (in VK_IMAGE_LAYOUT_GENERAL, see EnvMapVulkanComputeImage), pipeline and descriptor sets.
 */
class EnvMapVulkanPathTracer
{
public:
	static constexpr VkFormat Format = VK_FORMAT_R32G32B32A32_SFLOAT; // an average of thousands of samples

	// The push constants of PathTrace.glsl
	struct Push
	{
		glm::mat4 InverseViewProjection;
		glm::ivec4 Counts;
		glm::vec4 Params;
	};

	void Create();
	void Destroy();
	// Follows the viewport: a new image of this size (waits for the GPU), without samples. The set that shows it must be
	// written again (GetImageInfo).
	void Resize(uint32_t width, uint32_t height);

	// Before anything is recorded, in a frame that traces paths: the per-frame set's contents for the pass
	void Prepare(VkDescriptorSet frameDescriptorSet);
	// After EnvMapVulkanRayTracing::Record (rayTracingSet: set 3 of the ray tracing shaders). sceneVersion: a number that
	// changes whenever the scene does (meshes, materials, lights, environment). skyboxLod: the level of the environment
	// map the skybox is drawn with. sunShadows: the sun casts shadows.
	void Record(VkCommandBuffer commandBuffer, const EnvMapVulkanPathTracerSettings& settings, const glm::mat4& viewProjection,
		const glm::mat4& projection, uint64_t sceneVersion, float skyboxLod, VkDescriptorSet rayTracingSet, bool sunShadows);
	// The next Record starts the average over
	void Restart() { m_Samples = 0; }

	VkDescriptorImageInfo GetImageInfo() const { return m_Image.GetSampledInfo(m_Sampler); }
	// Paths per pixel in the image
	uint32_t GetSamples() const { return m_Samples; }
	uint32_t GetWidth() const { return m_Image.Width; }
	uint32_t GetHeight() const { return m_Image.Height; }

private:
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_Pipeline;        // on a device with ray tracing
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_FrameSet; // the pass's set 0: the per-frame set's contents
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_ImageSet; // its set 1: the image

	EnvMapVulkanComputeImage m_Image;
	VkSampler m_Sampler = VK_NULL_HANDLE; // nearest: shown at its own size
	uint32_t m_Samples = 0;
	uint64_t m_Version = 0; // of everything the image's samples were traced with
};
