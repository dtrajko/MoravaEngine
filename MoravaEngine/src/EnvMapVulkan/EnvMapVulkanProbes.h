#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>


/**
 * The probe volume of a frame: what the renderer reads from the scene's probe volume entity (see ProbeVolumeComponent).
 * A box of the world with a grid of probes in it, the first and the last of each axis on the box's sides.
 */
struct EnvMapVulkanProbeVolumeSettings
{
	// Probes along an axis: at least 2 (a cell needs its two ends); the upper limits keep the atlases inside the largest
	// image a GPU has to support (the visibility atlas is CountX * CountY * 18 texels wide)
	static constexpr int MinCount = 2;
	static constexpr glm::ivec3 MaxCounts = { 32, 16, 32 };

	bool Exists = false;                         // the scene has a probe volume
	bool Enabled = true;                         // its probes light the scene (off: the environment does, as without it)
	glm::vec3 Center = glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 Size = glm::vec3(10.0f, 5.0f, 10.0f); // meters
	glm::ivec3 Counts = { 8, 4, 8 };
	float NormalBias = 0.25f;                    // how far a lookup starts off the surface along its normal...
	float ViewBias = 0.25f;                      // ...and towards the viewer (fractions of the smallest probe spacing)
	bool ShowProbes = false;                     // editor: a ball at every probe, lit by that probe
	float ProbeRadius = 0.12f;                   // meters

	glm::ivec3 GetCounts() const { return glm::clamp(Counts, glm::ivec3(MinCount), MaxCounts); }
	glm::vec3 GetOrigin() const { return Center - Size * 0.5f; } // probe (0, 0, 0)
	glm::vec3 GetSpacing() const { return Size / glm::vec3(GetCounts() - 1); }
	bool LightsScene() const { return Exists && Enabled; }
};

/**
 * The probe volume on the GPU (SceneEnvMapVulkan): the grid of probes that holds the scene's indirect diffuse light (Phase
 * C of docs/rendering/SSAO_GI_RayTracing_Guide.html). Every probe stores, for every direction, the light arriving there
 * (irradiance) and how far it sees (visibility); a surface point is lit by the eight probes around it, each counted by how
 * well it sees the point (SampleProbeIrradiance in Include/Probes.glslh).
 *
 * The atlases (a tile per probe: the probes of a layer side by side, the layers in rows; see Include/ProbeOctahedral.glslh):
 *   irradiance  RGBA16F  10 x 10 texels per probe (8 x 8 and a border): irradiance / pi, to the power 1 / IrradianceGamma
 *   visibility  RG16F    18 x 18 texels per probe (16 x 16 and a border): the mean distance seen, and the mean of its square
 *   probe data  RGBA16F  1 texel per probe: xyz = its offset from the grid, w = 1 when it is in use
 * They stay in VK_IMAGE_LAYOUT_GENERAL: compute passes write them and the mesh shaders sample them (set 0, bindings 11,
 * 12 and 14), with no layout change in between.
 *
 * So far the probes hold the environment's light only (ProbeEnvFill.glsl): they don't know the scene yet. Lit by them the
 * image is the same as lit by the environment directly, which is the test that the storage and the sampling are right.
 * Capturing the scene into the probes is the next phase.
 *
 * Owns: the atlases, their samplers, the fill's compute pipeline and descriptor set, and the pipeline of the probe balls.
 */
class EnvMapVulkanProbes
{
public:
	static constexpr VkFormat IrradianceFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr VkFormat VisibilityFormat = VK_FORMAT_R16G16_SFLOAT;
	static constexpr VkFormat ProbeDataFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr int IrradianceTexels = 8;   // interior texels per side of a probe's tile, as in Include/ProbeOctahedral.glslh
	static constexpr int VisibilityTexels = 16;
	static constexpr float IrradianceGamma = 5.0f;

	// The ProbeVolume block of set 0 (binding 13, std140), see Include/FrameSet.glslh
	struct ProbeVolumeUB
	{
		glm::vec4 Origin;     // w: 1 when the probes light the scene
		glm::vec4 Spacing;    // w: the irradiance encoding gamma
		glm::ivec4 Counts;
		glm::vec4 Biases;     // x normal, y view
		glm::vec4 AtlasSizes; // xy irradiance, zw visibility
	};

	// sceneFramebuffer: the probe balls are drawn into its render pass
	void Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	void Destroy();

	// Call at the start of a frame, before anything is recorded. Makes the atlases fit the volume's probe counts and fills
	// them from the environment when they are new, or when the volume or the environment changed (environmentVersion: a
	// number that changes whenever the environment's irradiance cube gets new contents). Both wait for the GPU.
	// Returns true when the atlases were replaced: descriptor sets that point to them must be written again.
	bool Update(const EnvMapVulkanProbeVolumeSettings& settings, const VkDescriptorImageInfo& environmentIrradiance, float environmentRotation,
		uint32_t environmentVersion);

	// What the shaders need of the volume (of the settings given to Update)
	ProbeVolumeUB GetUniforms() const;
	// For descriptor sets that sample the atlases
	VkDescriptorImageInfo GetIrradianceInfo() const { return { m_LinearSampler, m_Irradiance.View, VK_IMAGE_LAYOUT_GENERAL }; }
	VkDescriptorImageInfo GetVisibilityInfo() const { return { m_LinearSampler, m_Visibility.View, VK_IMAGE_LAYOUT_GENERAL }; }
	VkDescriptorImageInfo GetProbeDataInfo() const { return { m_NearestSampler, m_ProbeData.View, VK_IMAGE_LAYOUT_GENERAL }; }

	// The probe balls (ShowProbes), inside the scene's render pass after the opaque meshes. frameDescriptorSet: the
	// per-frame set 0; cameraView: the camera's view matrix (the balls' squares face the camera).
	void RecordSpheres(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const glm::mat4& cameraView);
	bool ShowsSpheres() const { return m_Settings.Exists && m_Settings.ShowProbes; }
	uint32_t GetProbeCount() const { return (uint32_t)(m_Counts.x * m_Counts.y * m_Counts.z); }

private:
	struct Atlas
	{
		VkImage Image = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE;
		uint32_t Width = 0, Height = 0;
	};
	void CreateAtlas(Atlas& atlas, VkFormat format, uint32_t width, uint32_t height);
	void CreateAtlases(const glm::ivec3& counts);
	void DestroyAtlases();
	void FillFromEnvironment(const VkDescriptorImageInfo& environmentIrradiance, float environmentRotation);
	void CreateSpherePipeline(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);

	EnvMapVulkanProbeVolumeSettings m_Settings;
	glm::ivec3 m_Counts = glm::ivec3(0); // of the atlases

	Atlas m_Irradiance, m_Visibility, m_ProbeData;
	VkSampler m_LinearSampler = VK_NULL_HANDLE;
	VkSampler m_NearestSampler = VK_NULL_HANDLE;

	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_FillPipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_FillDescriptorSet;
	// What the atlases were last filled from: a difference means a new fill
	bool m_Filled = false;
	glm::vec3 m_FilledSpacing = glm::vec3(0.0f);
	float m_FilledRotation = 0.0f;
	uint32_t m_FilledEnvironmentVersion = 0;

	VkPipelineLayout m_SpherePipelineLayout = VK_NULL_HANDLE;
	VkPipeline m_SpherePipeline = VK_NULL_HANDLE;
};
