#pragma once

#include "EnvMapVulkanTemporalFilter.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>

class EnvMapVulkanGBuffer;


// The Environment panel's Reflections settings, saved with the scene (RenderSettings)
struct EnvMapVulkanReflectionSettings
{
	bool Enabled = true;
	float MaxRoughness = 0.5f;  // surfaces rougher than this keep the environment's reflection (their rays would be all
	                            // noise, and so blurred a reflection hardly shows what it reflects)
	bool Denoise = true;        // the area a pixel takes its reflection from grows with its roughness
	bool Temporal = true;       // averaged over the frames (and the rays' directions change every frame)
	bool HalfResolution = true; // traced at half the viewport's size (a quarter of the rays); off: a ray per pixel

	static constexpr float MinMaxRoughness = 0.05f;
	static constexpr float MaxMaxRoughness = 1.0f;
};

/**
 * Ray-traced reflections (SceneEnvMapVulkan, Phase H.3 of docs/rendering/SSAO_GI_RayTracing_Guide.html): what every
 * pixel's surface reflects, found by a ray against the scene, in place of the environment map's reflection. The
 * environment map shows the sky only: with it a room reflects the sky through its walls, and nothing in the scene shows
 * up in anything else.
 *
 * The result is the reflection image: per pixel of the viewport rgb = the reflected light, a = how much of it replaces
 * the environment's reflection (0 where no reflection was traced: surfaces above the max roughness, and everything while
 * the effect is off). It is bound in the mesh shaders' set 0 at binding 16 (u_ReflectionImage, GetReflectionInfo) and
 * read by the opaque meshes (IndirectLight in Include/PBRCommon.glslh).
 *
 * Record, between the G-buffer prepass and the scene pass, in the frame's command buffer, after the scene's acceleration
 * structures were built. Compute passes, the first two at half the viewport's size (or at its full size: a setting):
 *   1. ReflectionTrace    a ray per pixel in the reflection's direction, what it hits lit as ShadeSurfaceDiffuse does
 *                         (shadow rays, the probes' light) -> the raw reflection (noisy on rough surfaces), and the
 *                         pixel's linear depth
 *   2. the temporal filter (EnvMapVulkanTemporalFilter): the raw reflection + the G-buffer's motion -> its history
 *   3. ReflectionResolve  the history (or the raw reflection: the debug view) -> the reflection image, full size, each
 *                         pixel from the traced pixels of its own surface around it
 * Without ray tracing nothing is recorded and the image stays empty: the environment's reflection everywhere, as before.
 *
 * The trace lights what it hits with the per-frame set's contents (lights, probes, environment): a compute pipeline
 * can't use the mesh shaders' set 0, so it has its own, filled in Prepare.
 *
 * Owns its images (all in VK_IMAGE_LAYOUT_GENERAL, see EnvMapVulkanComputeImage), pipelines and descriptor sets, and an
 * empty 1 x 1 image for set 0s that have no reflections (the water's reflection pass: GetNoneInfo).
 */
class EnvMapVulkanReflections
{
public:
	static constexpr VkFormat Format = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr VkFormat DepthFormat = VK_FORMAT_R32_SFLOAT;

	// What the reflection image shows (the Debug View's steps; the scene is lit with what is shown)
	enum class Stage { Final, Raw };

	// The push constants of ReflectionTrace.glsl
	struct TracePush
	{
		glm::mat4 InverseViewProjection;
		glm::vec4 CameraPosition;
		glm::vec4 Params;
		glm::vec4 Depth;
	};
	// The push constants of ReflectionResolve.glsl
	struct ResolvePush
	{
		glm::vec4 Params;
		glm::vec4 Depth;
	};

	void Create();
	void Destroy();
	// Follows the G-buffer: call after it was created or resized, and when the Half Resolution setting changed
	// (IsHalfResolution). Waits for the GPU. The reflection image is a new one then: the sets that hold it must be
	// written again (GetReflectionInfo).
	void Resize(const EnvMapVulkanGBuffer& gbuffer, bool halfResolution);
	bool IsHalfResolution() const { return m_Scale == 2; }

	// Before anything is recorded, in a frame that traces reflections: the per-frame set's contents for the trace
	void Prepare(VkDescriptorSet frameDescriptorSet);
	// After the G-buffer prepass, before the scene pass. rayTracingSet: set 3 of the ray tracing shaders when rays can be
	// traced this frame (after EnvMapVulkanRayTracing::Record); null, or the setting off: the image is emptied (once).
	// sunShadows: the sun casts shadows.
	void Record(VkCommandBuffer commandBuffer, const EnvMapVulkanReflectionSettings& settings, EnvMapVulkanGBuffer& gbuffer,
		const glm::mat4& viewProjection, const glm::mat4& projection, const glm::vec3& cameraPosition, Stage stage,
		VkDescriptorSet rayTracingSet, bool sunShadows);
	// The last Record traced reflections
	bool IsTraced() const { return m_Traced; }

	// For the mesh shaders' set 0. Before the first Resize: the empty image.
	VkDescriptorImageInfo GetReflectionInfo() const;
	VkDescriptorImageInfo GetNoneInfo() const { return m_None.GetSampledInfo(m_Sampler); }

	uint32_t GetWidth() const { return m_Width; }   // of the trace
	uint32_t GetHeight() const { return m_Height; }

private:
	void WriteDescriptors(const EnvMapVulkanGBuffer& gbuffer);

	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_TracePipeline;   // on a device with ray tracing
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_ResolvePipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_TraceFrameSet; // the trace's set 0: the per-frame set's contents
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_TraceSet;      // its set 1: the G-buffer, the raw reflection, the depth
	// What the resolve reads: the raw reflection, the temporal filter's two history images
	enum { ResolveRaw = 0, ResolveHistory0, ResolveHistory1, ResolveSetCount };
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_ResolveSets[ResolveSetCount];
	EnvMapVulkanTemporalFilter m_TemporalFilter;

	EnvMapVulkanComputeImage m_Raw;        // the trace's size
	EnvMapVulkanComputeImage m_Depth;      // the trace's size, linear
	EnvMapVulkanComputeImage m_Reflection; // the viewport's size
	EnvMapVulkanComputeImage m_None;       // 1 x 1, empty
	VkSampler m_Sampler = VK_NULL_HANDLE;  // nearest
	uint32_t m_Width = 0, m_Height = 0;
	uint32_t m_FullWidth = 0, m_FullHeight = 0;
	uint32_t m_Scale = 2; // full-size pixels per pixel of the trace, along an axis
	uint32_t m_Frame = 0;
	bool m_IsEmpty = true; // as created, or emptied since the reflections were turned off
	bool m_Traced = false;
};
