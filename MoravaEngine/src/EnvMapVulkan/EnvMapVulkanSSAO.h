#pragma once

#include "EnvMapVulkanTemporalFilter.h"

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>

class EnvMapVulkanGBuffer;


// The Environment panel's Ambient Occlusion settings, saved with the scene (RenderSettings)
struct EnvMapVulkanSSAOSettings
{
	bool Enabled = true;
	float Radius = 0.5f;   // world units: how far from a point the geometry still shades it. Small with a probe volume,
	                       // which already has the occlusion of everything larger
	float Power = 1.5f;    // the result to this power: above 1 darker, below 1 lighter
	float Falloff = 0.6f;  // the part of the radius over which a sample stops counting
	int Slices = 2;        // directions looked in per pixel and frame
	int Steps = 8;         // depth lookups per direction and side
	bool Denoise = true;   // the 5 x 5 blur that keeps edges
	bool Temporal = true;  // averaged over the frames (and the noise changes every frame)
	bool HalfResolution = true; // the passes at half the viewport's size (a quarter of the work); off: at its full size
	// With ray tracing: the occlusion from short rays against the scene's geometry instead of from the depth buffer
	// (Slices and Steps are the depth buffer's way and don't apply then)
	bool RayTraced = true;
	int Rays = 2;          // rays per pixel and frame

	static constexpr int MaxSlices = 4;
	static constexpr int MaxSteps = 16;
	static constexpr int MaxRays = 8;
};

/**
 * Screen-space ambient occlusion (SceneEnvMapVulkan, Phase F of docs/rendering/SSAO_GI_RayTracing_Guide.html): how much
 * of the light from all around reaches each pixel's surface point past the geometry close to it, from the G-buffer
 * (GTAO). The probe volume can't know that: its probes stand meters apart.
 *
 * The result is the AO image: one value per pixel of the viewport (R8: 1 = open, 0 = closed in), bound in the mesh
 * shaders' set 0 at binding 15 (u_AOImage, GetAOInfo) and used on the indirect light only. A later ray-traced occlusion
 * writes the same image.
 *
 * Record, between the G-buffer prepass and the scene pass, in the frame's command buffer. Compute passes, all but the
 * last at half the viewport's size (or at its full size: the Half Resolution setting):
 *   1. SSAODepth      the depth buffer -> linear depth (level 0 of an R32F image)
 *   2. SSAODepthMip   its smaller levels, one dispatch each
 *   3. SSAOGTAO       linear depth + the G-buffer's normals -> raw AO (noisy)
 *   4. SSAODenoise    raw AO -> denoised AO (a blur that keeps edges)
 *   5. the temporal filter (EnvMapVulkanTemporalFilter): denoised AO + the G-buffer's motion -> its history
 *   6. SSAOUpsample   the history (or an earlier step's image: the debug views) -> the AO image, full size
 * The depth buffer is sampled by 1 and 6: Record changes its layout to read-only and back to the attachment layout the
 * scene pass expects.
 * With ray tracing (Record's rayTracingSet; Phase H.2 of the guide) steps 2 and 3 are replaced by SSAORayTraced.glsl: a
 * few short rays per pixel against the scene's acceleration structures, into the same raw AO image. The rest is the same.
 *
 * Owns its images (all in VK_IMAGE_LAYOUT_GENERAL, see EnvMapVulkanComputeImage), pipelines and descriptor sets, and a
 * white 1 x 1 image for set 0s that have no ambient occlusion (the water's reflection pass: GetWhiteInfo).
 */
class EnvMapVulkanSSAO
{
public:
	static constexpr VkFormat AOFormat = VK_FORMAT_R8_UNORM;
	static constexpr VkFormat DepthFormat = VK_FORMAT_R32_SFLOAT;
	static constexpr uint32_t MaxDepthLevels = 5;

	// What the AO image shows (the Debug View's steps; the scene is lit with what is shown)
	enum class Stage { Final, Denoised, Raw };

	// The push constants of the SSAO shaders (Include/SSAO.glslh)
	struct Push
	{
		glm::mat4 View;
		glm::vec4 Projection;
		glm::vec4 Depth;
		glm::vec4 Params;
		glm::ivec4 Counts;
	};

	void Create();
	void Destroy();
	// Follows the G-buffer: call after it was created or resized, and when the Half Resolution setting changed
	// (IsHalfResolution). Waits for the GPU. The AO image is a new one then: the sets that hold it must be written again
	// (GetAOInfo).
	void Resize(const EnvMapVulkanGBuffer& gbuffer, bool halfResolution);
	bool IsHalfResolution() const { return m_Scale == 2; }
	// After the G-buffer prepass, before the scene pass. Off: the AO image is cleared to white (once).
	// rayTracingSet: set 3 of the ray tracing shaders when rays can be traced this frame (EnvMapVulkanRayTracing, after
	// its Record); with it and the Ray Traced setting the occlusion is traced. Null: from the depth buffer (GTAO).
	void Record(VkCommandBuffer commandBuffer, const EnvMapVulkanSSAOSettings& settings, EnvMapVulkanGBuffer& gbuffer,
		const glm::mat4& view, const glm::mat4& projection, Stage stage, VkDescriptorSet rayTracingSet = VK_NULL_HANDLE);
	// The last Record traced rays
	bool IsRayTraced() const { return m_RayTraced; }

	// For the mesh shaders' set 0. Before the first Resize: the white image.
	VkDescriptorImageInfo GetAOInfo() const;
	VkDescriptorImageInfo GetWhiteInfo() const { return m_White.GetSampledInfo(m_Sampler); }

	uint32_t GetWidth() const { return m_Width; }   // of the passes
	uint32_t GetHeight() const { return m_Height; }

private:
	struct Pass
	{
		H2M::RefH2M<H2M::VulkanComputePipelineH2M> Pipeline;
		void Create(const char* shaderName);
	};
	void WriteDescriptors(const EnvMapVulkanGBuffer& gbuffer);

	Pass m_DepthPass, m_DepthMipPass, m_GTAOPass, m_DenoisePass, m_UpsamplePass;
	Pass m_RayTracedPass; // on a device with ray tracing
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_RayTracedSet;
	bool m_RayTraced = false;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DepthSet;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DepthMipSets[MaxDepthLevels - 1]; // [i] writes level i + 1
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_GTAOSet;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DenoiseSet;
	// What the upsample reads: the raw AO, the denoised AO, the temporal filter's two history images
	enum { UpsampleRaw = 0, UpsampleDenoised, UpsampleHistory0, UpsampleHistory1, UpsampleSetCount };
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_UpsampleSets[UpsampleSetCount];
	EnvMapVulkanTemporalFilter m_TemporalFilter;

	EnvMapVulkanComputeImage m_Depth;     // half size, with levels
	EnvMapVulkanComputeImage m_Raw;       // half size
	EnvMapVulkanComputeImage m_Denoised;  // half size
	EnvMapVulkanComputeImage m_AO;        // the viewport's size
	EnvMapVulkanComputeImage m_White;
	VkSampler m_Sampler = VK_NULL_HANDLE; // nearest, all levels
	uint32_t m_Width = 0, m_Height = 0;
	uint32_t m_FullWidth = 0, m_FullHeight = 0;
	uint32_t m_Scale = 2; // full-size pixels per pixel of the passes, along an axis
	uint32_t m_DepthLevels = 1;
	uint32_t m_Frame = 0;
	bool m_AOIsWhite = true; // as created, or cleared since the occlusion was turned off
};
