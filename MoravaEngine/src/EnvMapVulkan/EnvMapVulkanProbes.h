#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanComputePipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <random>
#include <vector>


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
	// The most probes one frame updates: each gets a slot of the capture cubes and a row of the ray buffer
	static constexpr int MaxProbesPerFrame = 32;
	// Rays per probe and update: the ray buffer is as wide as the most
	static constexpr int MinRaysPerProbe = 32;
	static constexpr int MaxRaysPerProbe = 256;
	enum ShowMode { ShowIrradiance = 0, ShowVisibility };

	bool Exists = false;                         // the scene has a probe volume
	bool Enabled = true;                         // its probes light the scene (off: the environment does, as without it)
	glm::vec3 Center = glm::vec3(0.0f, 2.5f, 0.0f);
	glm::vec3 Size = glm::vec3(10.0f, 5.0f, 10.0f); // meters
	glm::ivec3 Counts = { 8, 4, 8 };
	float NormalBias = 0.25f;                    // how far a lookup starts off the surface along its normal...
	float ViewBias = 0.25f;                      // ...and towards the viewer (fractions of the smallest probe spacing)
	// Capturing the scene into the probes
	bool AutoUpdate = true;                      // the probes follow the scene: updated whenever something in it changed
	int ProbesPerFrame = 8;                      // probes updated per frame while they follow a change (1 to MaxProbesPerFrame)
	int RaysPerProbe = 192;                      // directions a probe looks in per update
	float Hysteresis = 0.9f;                     // how much of a probe's old value an update keeps: higher is calmer and slower
	bool ShowProbes = false;                     // editor: a ball at every probe, lit by that probe
	float ProbeRadius = 0.12f;                   // meters
	int Show = ShowIrradiance;                   // what the balls show: the light a probe holds, or how far it sees

	int GetProbesPerFrame() const { return glm::clamp(ProbesPerFrame, 1, MaxProbesPerFrame); }
	int GetRaysPerProbe() const { return glm::clamp(RaysPerProbe, MinRaysPerProbe, MaxRaysPerProbe); }
	float GetHysteresis() const { return glm::clamp(Hysteresis, 0.0f, 0.99f); }

	glm::ivec3 GetCounts() const { return glm::clamp(Counts, glm::ivec3(MinCount), MaxCounts); }
	glm::vec3 GetOrigin() const { return Center - Size * 0.5f; } // probe (0, 0, 0)
	glm::vec3 GetSpacing() const { return Size / glm::vec3(GetCounts() - 1); }
	bool LightsScene() const { return Exists && Enabled; }
};

/**
 * The probe volume on the GPU (SceneEnvMapVulkan): the grid of probes that holds the scene's indirect diffuse light (Phases
 * C and D of docs/rendering/SSAO_GI_RayTracing_Guide.html). Every probe stores, for every direction, the light arriving there
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
 * New atlases are filled with the environment's light (ProbeEnvFill.glsl): the probes don't know the scene yet, and lit by
 * them the image is the same as lit by the environment directly. Then the probes capture the scene, a batch per frame:
 *   1. capture   the renderer draws the meshes into a small cube map around each probe of the batch (a slot each; 6
 *                render passes; ProbeCapture_Static.glsl / _Anim.glsl): the light every surface sends to the probe
 *                (ShadeSurfaceDiffuse: the lights, and the light the probes already hold, so light bounces on from
 *                update to update) and its distance. The sun's shadow there comes from a shadow map of the whole
 *                volume, which the renderer draws (SetCaptureSun): the scene's cascades only cover the camera's view.
 *   2. resample  ProbeResample.glsl reads each cube in the directions of the probe's rays, into the ray buffer
 *                (RGBA16F, a row per slot, a texel per ray: rgb = the light arriving, a = the distance; a ray that
 *                hit nothing gets the environment's light). Ray tracing will replace steps 1 and 2 and fill the same
 *                buffer; the rest stays.
 *   3. blend     ProbeBlendIrradiance.glsl and ProbeBlendVisibility.glsl mix the rays into the probes' tiles, keeping
 *                Hysteresis of the old values; ProbeBorders.glsl rewrites the tiles' border texels.
 * Which probes: all of them in turn (the batch is the next run of probes; a round is once over all of them), as long as
 * something is left to do. A change of the scene (the caller's scene version) or of the volume's placement asks for
 * PropagationRounds rounds that replace what the probes hold (hysteresis 0: each is one more bounce of the new light and
 * one less of the old; see GetRoundHysteresis), then rounds that are averaged until the hysteresis is the setting's
 * (GetSettleRounds). Animation alone (the caller's animation version) only asks for rounds with the setting's
 * hysteresis, so a scene that never stops moving isn't lit by single noisy rounds. Bake asks for BakeRounds at the most
 * probes per frame. Then the updates stop.
 *
 * Owns: the atlases, their samplers, the fill's compute pipeline and descriptor set, the pipeline of the probe balls, and
 * for the updates the capture cubes with their render pass, framebuffers and pipelines, the ray buffer and the compute
 * pipelines with their descriptor sets.
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
	static constexpr uint32_t CaptureResolution = 32; // of a cube face
	static constexpr uint32_t CaptureSlots = EnvMapVulkanProbeVolumeSettings::MaxProbesPerFrame;
	static constexpr VkFormat CaptureRadianceFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr VkFormat CaptureDistanceFormat = VK_FORMAT_R32_SFLOAT;
	static constexpr VkFormat CaptureDepthFormat = VK_FORMAT_D32_SFLOAT;
	static constexpr VkFormat RayBufferFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
	static constexpr float CaptureMissDistance = 10000.0f; // ProbeMissDistance in Include/ProbeUpdate.glslh
	static constexpr uint32_t BakeRounds = 32;             // updates of every probe in a bake: half let the light travel, half are averaged
	static constexpr uint32_t PropagationRounds = 12;      // rounds after a change that replace what the probes hold

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
	// them from the environment when they are new (and, while the probes haven't captured the scene, when the volume or
	// the environment changed; environmentVersion: a number that changes whenever the environment's cubes get new
	// contents). Both wait for the GPU. Then decides which probes this frame updates (HasCapture).
	// environmentRadiance: the environment's reflection cube with its mip levels (what a ray that hits nothing sees).
	// sceneVersion: a number that changes whenever something the probes capture changed, other than by animation: the
	// meshes, their materials, the lights, the environment. animationVersion: the same for what moves by itself (the
	// skinned models and their poses).
	// Returns true when the atlases were replaced: descriptor sets that point to them must be written again.
	bool Update(const EnvMapVulkanProbeVolumeSettings& settings, const VkDescriptorImageInfo& environmentIrradiance,
		const VkDescriptorImageInfo& environmentRadiance, float environmentRotation, uint32_t environmentVersion, uint64_t sceneVersion,
		uint64_t animationVersion);

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

	// ---- This frame's update (after Update), recorded by the renderer in this order:
	//   SetCaptureSun; for every slot and face: BeginCaptureFace, the meshes, EndCaptureFace; RecordBlend
	// Probes to update this frame: 0 when nothing is left to do
	bool HasCapture() const { return m_BatchCount > 0; }
	uint32_t GetCaptureCount() const { return m_BatchCount; }
	glm::vec3 GetCaptureProbePosition(uint32_t slot) const;

	// The push constants of the capture pipelines (Include/ProbeCapture.glslh), for both stages
	struct CapturePush
	{
		glm::mat4 FaceViewProjection;
		glm::vec4 TransformRows[3];      // the mesh's transform: the columns of its transpose
		glm::vec4 ProbePositionMaterial; // xyz the probe, w the material's index in the material buffer
	};
	static constexpr VkShaderStageFlags CapturePushStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	// Set 0: the per-frame set; set 1: GetCaptureSet; set 2 (skinned): the model's bone set. The two layouts agree in
	// sets 0 and 1 and the push constants, so those stay bound from one pipeline to the other.
	VkPipeline GetCapturePipeline(bool skinned) const { return m_CapturePipelines[skinned ? 1 : 0]; }
	VkPipelineLayout GetCaptureLayout(bool skinned) const { return m_CaptureLayouts[skinned ? 1 : 0]; }
	VkDescriptorSet GetCaptureSet() const { return m_CaptureSet.DescriptorSets[0]; }
	static constexpr uint32_t CaptureDescriptorSet = 1;

	// The sun's shadow map of the probe volume (one layer, a comparison sampler), which the capture's fragment stage
	// reads. Once, and again when the map is replaced (nothing in flight may use the set then).
	void SetCaptureSunShadowMap(const VkDescriptorImageInfo& shadowMap);
	// Every frame with a capture: the map's view-projection; drawn: the map was drawn this frame (false: no sun shadows);
	// texelWorldSize: the world size of one of its texels; normalBias: in texels, as the scene's shadow settings
	void SetCaptureSun(const glm::mat4& viewProjection, bool drawn, float texelWorldSize, uint32_t resolution, float normalBias);

	// A render pass into one face (0 to 5: +X, -X, +Y, -Y, +Z, -Z) of a slot's cube, cleared to "nothing seen", with its
	// viewport and scissor set. Returns the face's view-projection (CapturePush::FaceViewProjection).
	glm::mat4 BeginCaptureFace(VkCommandBuffer commandBuffer, uint32_t slot, uint32_t face);
	void EndCaptureFace(VkCommandBuffer commandBuffer);
	// After the captures: the compute passes that turn the cubes into rays and blend them into the atlases
	void RecordBlend(VkCommandBuffer commandBuffer);

	// ---- For the editor
	// Updates every probe BakeRounds times, at the most probes per frame, starting with the next frame
	void RequestBake() { m_BakeRequested = true; }
	// Back to the environment's light, as new atlases (the next Update fills them); with AutoUpdate the capture starts over
	void RequestReset() { m_ResetRequested = true; }
	struct UpdateStatus
	{
		bool Supported = true;     // false: the GPU has no cube map arrays, the probes keep the environment's light
		bool Captured = false;     // the probes hold the scene's light (false: the environment's only)
		bool Baking = false;
		float BakeProgress = 0.0f; // 0 to 1
		uint32_t UpdatesLeft = 0;  // probe updates until the probes have followed the last change
		bool OutOfDate = false;    // something changed and nothing updates the probes (AutoUpdate is off)
		uint32_t ProbesThisFrame = 0;
	};
	UpdateStatus GetStatus() const;
	// Rounds over all probes until a probe that keeps hysteresis of its value per update has lost its old value (to about
	// 2 percent)
	static uint32_t GetSettleRounds(float hysteresis);
	// The hysteresis of a round (counted from the last change): 0 for the first propagationRounds, then rising, as the
	// rounds are averaged, to settledHysteresis
	static float GetRoundHysteresis(uint32_t round, uint32_t propagationRounds, float settledHysteresis);

private:
	struct Image
	{
		VkImage Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE; // all layers
		uint32_t Width = 0, Height = 0;
	};
	// layers > 1 with cube: a cube map array (6 layers per cube). The view is of all layers.
	void CreateImage(Image& image, VkFormat format, uint32_t width, uint32_t height, VkImageUsageFlags usage, uint32_t layers = 1, bool cube = false);
	void DestroyImage(Image& image);
	void CreateAtlases(const glm::ivec3& counts);
	void DestroyAtlases();
	void FillFromEnvironment(const VkDescriptorImageInfo& environmentIrradiance, float environmentRotation);
	void CreateSpherePipeline(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	void CreateCaptureResources();
	void DestroyCaptureResources();
	void CreateCapturePipeline(bool skinned);
	// Points the update's compute sets to the atlases, the capture cubes, the ray buffer and the environment
	void WriteUpdateDescriptors(const VkDescriptorImageInfo& environmentRadiance);
	// Decides this frame's batch. restarted: the atlases were just filled from the environment.
	void ScheduleUpdate(uint64_t sceneVersion, uint64_t animationVersion, bool restarted);

	EnvMapVulkanProbeVolumeSettings m_Settings;
	glm::ivec3 m_Counts = glm::ivec3(0); // of the atlases

	Image m_Irradiance, m_Visibility, m_ProbeData;
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

	// ---- The updates
	bool m_CaptureSupported = false; // cube map arrays (the imageCubeArray device feature)
	// A cube per slot (6 layers each): between the captures in SHADER_READ_ONLY_OPTIMAL. One depth image for all faces.
	Image m_CaptureRadiance, m_CaptureDistance, m_CaptureDepth;
	std::vector<VkImageView> m_CaptureLayerViews;     // per layer: the radiance's, then the distance's
	std::vector<VkFramebuffer> m_CaptureFramebuffers; // per layer
	VkRenderPass m_CaptureRenderPass = VK_NULL_HANDLE;
	VkPipeline m_CapturePipelines[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE }; // static, skinned
	VkPipelineLayout m_CaptureLayouts[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_CaptureSet; // set 1 of the capture pipelines
	Image m_RayBuffer; // MaxRaysPerProbe x CaptureSlots, always VK_IMAGE_LAYOUT_GENERAL
	H2M::RefH2M<H2M::VulkanComputePipelineH2M> m_ResamplePipeline, m_BlendIrradiancePipeline, m_BlendVisibilityPipeline, m_BorderPipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_ResampleSet, m_BlendIrradianceSet, m_BlendVisibilitySet, m_BorderSet;
	bool m_UpdateSetsWritten = false;
	uint32_t m_UpdateSetsEnvironmentVersion = 0;

	// The schedule
	bool m_Captured = false;        // an update was blended into the atlases since they were filled
	uint64_t m_CapturedVersion = 0; // of the scene and the volume's placement, as last seen
	uint64_t m_CapturedAnimationVersion = 0;
	bool m_Changed = false;         // the version changed and no updates were asked for yet
	bool m_Animated = false;        // the same for the animation version
	uint32_t m_UpdatesLeft = 0;     // probe updates still to do for the last change
	uint32_t m_SinceChange = 0;     // probe updates done since the last change (not animation): its round decides the hysteresis
	uint32_t m_BakeLeft = 0, m_BakeTotal = 0;
	bool m_BakeRequested = false, m_ResetRequested = false;
	uint32_t m_Cursor = 0;          // the next probe to update (probes are counted along x, then y, then z)
	// This frame's batch: m_BatchCount probes from m_BatchFirst on (wrapping around), slot i = probe m_BatchFirst + i
	uint32_t m_BatchFirst = 0, m_BatchCount = 0;
	float m_BatchHysteresis = 0.0f;
	glm::mat3 m_BatchRotation = glm::mat3(1.0f); // of the ray directions
	float m_EnvironmentRotation = 0.0f;
	std::mt19937 m_Random{ 1234567u };
};
