#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>


namespace H2M
{
	class FramebufferH2M;
	class PipelineH2M;
	class VertexBufferH2M;
	class IndexBufferH2M;
	class Texture2D_H2M;
}

// std140 layout of the SceneData uniform block (set 0, binding 1, Include/FrameSet.glslh): the camera position, the
// environment rotation, the scene's water (the light under it, see FillWaterSceneData) and the water volume the meshes
// are seen through (off in the scene pass)
struct EnvMapVulkanSceneDataGPU
{
	glm::vec3 CameraPosition = glm::vec3(0.0f);
	float EnvMapRotation = 0.0f;
	glm::vec4 WaterVolumeBounds = glm::vec4(0.0f);     // center x, center z, half size x, half size z
	glm::vec4 WaterVolumeParams = glm::vec4(0.0f);     // x = height, y = 1 when the volume is on, z = 1 when the scene has water
	glm::vec4 WaterVolumeAbsorption = glm::vec4(0.0f); // rgb per meter
	glm::vec4 WaterVolumeScatter = glm::vec4(0.0f);    // rgb
	glm::vec4 CausticsRegion = glm::vec4(0.0f);        // the caustics map's corner (x, z), 1 / its size, 1 / fade width (map units)
	glm::vec4 CausticsParams = glm::vec4(0.0f);        // x = strength (0: no caustics this frame), y = focus depth
};
static_assert(sizeof(EnvMapVulkanSceneDataGPU) == 112, "EnvMapVulkanSceneDataGPU must match SceneData in Include/FrameSet.glslh");

/**
 * The water plane of SceneEnvMapVulkan: one flat, axis-aligned rectangle at a height (Resources/Shaders/Water.glsl).
 * There is at most one per scene; it exists while Enabled is set (Add Water / Remove Water in the Water panel).
 */
struct EnvMapVulkanWaterSettings
{
	bool Enabled = false;
	glm::vec2 Center = glm::vec2(0.0f);       // world X and Z
	glm::vec2 Size = glm::vec2(40.0f);        // along X and Z
	float Height = 0.0f;                      // world Y of the surface

	// Waves: two layers of the same normal map, at different scales, moving in different directions
	float WaveDirection = 30.0f;  // degrees around Y; the second layer moves 70 degrees off it
	float WaveSpeed = 0.5f;       // world units per second (the second layer is a bit slower)
	float WaveStrength = 0.25f;   // steepness of the waves
	float WaveScale1 = 7.0f;      // world size of one normal map tile, layer 1 (larger waves)
	float WaveScale2 = 2.5f;      // layer 2 (ripples)

	// Swell: Gerstner waves that move the surface's vertices (the normal map layers above only shade it). Four waves around
	// WaveDirection, the longest SwellLength long and the others shorter, each traveling at the speed of a real deep water
	// wave of its length (longer waves are faster).
	float SwellHeight = 0.25f;    // meters: the most the surface rises above its level (0: flat)
	float SwellLength = 12.0f;    // meters: the wavelength of the longest wave
	float SwellSteepness = 0.5f;  // 0: round waves; 1: sharp crests (the surface moves toward them, the troughs widen)
	bool Wireframe = false;       // draws the surface's grid over it, where the waves moved it

	// Look
	glm::vec3 ScatterColor = glm::vec3(0.012f, 0.045f, 0.055f); // light the water body sends back up (linear)
	float Roughness = 0.06f;            // the sun highlight's size and the reflection's blur
	float ReflectionStrength = 1.0f;    // 1 = physically based

	// Seeing into the water (refraction): light is absorbed along its path through the water, red first (Beer-Lambert)
	glm::vec3 Transmittance = glm::vec3(0.55f, 0.85f, 0.88f); // the part of each color that is left after Clarity meters
	float Clarity = 4.0f;               // meters (larger: clearer water, the bottom stays visible deeper)
	float RefractionStrength = 1.0f;    // how much the waves bend the view into the water
	float EdgeSoftness = 0.3f;          // meters of water over which the surface fades in at the shore
	float FoamAmount = 0.6f;            // foam along the shore and around objects (0: none)
	float FoamWidth = 0.4f;             // meters of water depth that get foam

	// Seen from below: 0 is physical (the sky only within Snell's window, about 49 degrees around the vertical, a mirror
	// of the water outside it); at 0.5 the window covers the whole sky and only grazing views are mirrored; at 1 the
	// mirror fades to a tenth too, so the surface is see-through up to the horizon. The default 0.48 looks natural:
	// nearly the whole sky, with a trace of the mirror left near the horizon.
	float TransparencyFromBelow = 0.48f;

	// Planar reflection: the scene drawn again from the camera mirrored in the water plane (the environment map fills in
	// where that image has nothing: the sky, and what is outside the view)
	bool PlanarReflection = true;
	uint32_t ReflectionDivisor = 2;     // the reflection image is the viewport size / this (1 full, 2 half, 4 quarter)
	float ReflectionDistortion = 1.0f;  // how much the waves bend the reflection

	// Caustics: the waves focus the sun's light into bright lines on what is under the water (Resources/Shaders/WaterCaustics.glsl).
	// They are computed over a square around the camera (CausticsArea meters across, kept over the water) and fade out at
	// its edges.
	bool Caustics = true;
	float CausticsStrength = 1.0f;      // 1 = as the waves focus the light; more exaggerates the pattern
	float CausticsFocus = 2.0f;         // meters under the surface where the pattern is computed (deeper: sharper, brighter lines)
	float CausticsArea = 40.0f;         // meters: the size of the square around the camera that gets caustics

	// The unit square of the water mesh -> the rectangle in the world
	glm::mat4 GetTransform() const;
};

/**
 * The water's GPU side: pipeline, mesh, normal map and descriptor set 1 (set 0 is the per-frame set of the PBR shaders,
 * bound by the caller's geometry pass), and the copy of the opaque scene the water looks into.
 *
 * Per frame, in the scene framebuffer (created with CopySource): the opaque meshes and the sky are drawn, the render pass
 * ends, CopyScene copies its color and depth, the continue render pass begins and Record draws the water. The water reads
 * the copies: the scene through the surface (refraction) and how much water is in front of it (absorption, edges, foam).
 *
 * Planar reflection, before the scene pass: BeginReflectionPass starts the reflection framebuffer's render pass and returns
 * its per-frame descriptor set (set 0 with the mirrored camera); the caller binds it and draws the meshes with the usual
 * pipelines; EndReflectionPass ends it. The mirrored view is also flipped vertically: a mirror reverses the winding of
 * the triangles, the flip turns it back, so back-face culling keeps working (the water samples the image flipped).
 * From above the water it holds what is above the surface; from under the water what is under it (the mirror the
 * surface is outside Snell's window), with the water between the surface and each mesh applied by the PBR shaders.
 */
class EnvMapVulkanWater
{
public:
	// targetFramebuffer: the HDR scene framebuffer the water is drawn into (after the opaque meshes)
	void Create(H2M::RefH2M<H2M::FramebufferH2M> targetFramebuffer);
	void Destroy();
	bool IsValid() const { return (bool)m_Pipeline; }

	// Recreates the scene copies and the reflection image for a new size of the scene framebuffer (call after resizing it)
	void Resize(uint32_t width, uint32_t height);

	// Moves the waves and writes the settings, the mirrored camera and the caustics for this frame. view, projection and
	// cameraPosition: the camera's; sunDirection: toward the sun (pointing down when the sun is off); frameDescriptorSet:
	// the main per-frame set (set 0), whose environment, light, shadow and caustics bindings the reflection's set 0 shares.
	void Update(const EnvMapVulkanWaterSettings& settings, float deltaTime, const glm::mat4& view, const glm::mat4& projection,
		const glm::vec3& cameraPosition, float envMapRotation, const glm::vec3& sunDirection, VkDescriptorSet frameDescriptorSet);

	// This frame draws the planar reflection (it is enabled; from above the water or from under it)
	bool IsReflectionActive() const { return m_ReflectionActive; }
	// Begins the reflection render pass (outside any render pass) and returns the reflection's per-frame set (set 0)
	VkDescriptorSet BeginReflectionPass(VkCommandBuffer commandBuffer);
	void EndReflectionPass(VkCommandBuffer commandBuffer);
	// Outside a render pass: copies the scene framebuffer's color and depth into the textures the water samples, and leaves
	// the framebuffer's attachments in the layouts its continue render pass expects
	void CopyScene(VkCommandBuffer commandBuffer, H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	// Inside the scene's continue render pass, before Record: the water volume over the scene (a full-screen pass, see
	// WaterFog.glsl): the underwater fog, and the water seen from the side or across the waterline
	void RecordVolume(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet);
	// Draws the water inside the scene's render pass (and its wireframe, when the settings ask for it, with lines
	// wireframeLineWidth pixels wide); frameDescriptorSet is the per-frame set (set 0)
	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const EnvMapVulkanWaterSettings& settings,
		float wireframeLineWidth = 1.0f);

	// This frame draws the caustics (they are enabled and the sun is up)
	bool IsCausticsActive() const { return m_CausticsActive; }
	// Outside a render pass, before the passes that draw the meshes: renders this frame's caustics map
	void RecordCaustics(VkCommandBuffer commandBuffer);
	// The caustics map, for the per-frame set (set 0, binding 10); valid from Create on, whether the water exists or not
	const VkDescriptorImageInfo& GetCausticsDescriptorInfo() const { return m_CausticsDescriptor; }

private:
	H2M::RefH2M<H2M::PipelineH2M> m_Pipeline;
	H2M::RefH2M<H2M::PipelineH2M> m_VolumePipeline;              // WaterFog.glsl
	H2M::RefH2M<H2M::PipelineH2M> m_WireframePipeline;           // WaterWireframe.glsl
	H2M::RefH2M<H2M::VertexBufferH2M> m_FullscreenTriangle;     // for the volume pass
	H2M::RefH2M<H2M::VertexBufferH2M> m_VertexBuffer;           // the surface's grid over the unit square
	H2M::RefH2M<H2M::IndexBufferH2M> m_IndexBuffer;
	uint32_t m_IndexCount = 0;
	float m_SwellPhases[4] = {};                                 // radians, advanced every frame (wrapped)
	H2M::RefH2M<H2M::Texture2D_H2M> m_NormalMap;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;
	glm::vec4 m_WaveOffsets = glm::vec4(0.0f); // in normal map tiles: layer 1 in xy, layer 2 in zw

	// The copy of the opaque scene (color, and depth in the depth format of the scene framebuffer)
	struct CopyImage
	{
		VkImage Image = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE;
		VkFormat Format = VK_FORMAT_UNDEFINED;
	};
	void CreateCopyImage(CopyImage& copy, VkFormat format, VkImageAspectFlags aspect);
	void DestroyCopyImages();
	void WriteSceneCopyDescriptors();
	CopyImage m_SceneColor, m_SceneDepth;
	uint32_t m_CopyWidth = 0, m_CopyHeight = 0;
	VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED, m_DepthFormat = VK_FORMAT_UNDEFINED;
	VkSampler m_ColorSampler = VK_NULL_HANDLE; // linear (the distorted refraction)
	VkSampler m_DepthSampler = VK_NULL_HANDLE; // nearest (depth values are not filtered)

	// Planar reflection: its framebuffer (the scene framebuffer's formats, so the mesh pipelines draw into it), its
	// per-frame set 0 (allocated with the PBR shader's layout) and the two buffers that differ from the main set's
	void CreateReflection();
	void ResizeReflection();
	void WriteReflectionDescriptor();
	H2M::RefH2M<H2M::FramebufferH2M> m_ReflectionFramebuffer;
	uint32_t m_ReflectionDivisor = 2;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_ReflectionFrameSet;
	H2M::VulkanShaderH2M::UniformBufferH2M m_ReflectionCamera;    // binding 0: the mirrored view projection
	H2M::VulkanShaderH2M::UniformBufferH2M m_ReflectionSceneData; // binding 1: the mirrored camera position
	bool m_ReflectionActive = false;

	// Caustics (WaterCaustics.glsl): the map (R16F, additive), its render pass, framebuffer and pipeline, the normal map's
	// set, and this frame's push constants (see Update)
	void CreateCaustics();
	void DestroyCaustics();
	VkImage m_CausticsImage = VK_NULL_HANDLE;
	VkDeviceMemory m_CausticsMemory = VK_NULL_HANDLE;
	VkImageView m_CausticsView = VK_NULL_HANDLE;
	VkSampler m_CausticsSampler = VK_NULL_HANDLE;
	VkRenderPass m_CausticsRenderPass = VK_NULL_HANDLE;
	VkFramebuffer m_CausticsFramebuffer = VK_NULL_HANDLE;
	VkPipelineLayout m_CausticsLayout = VK_NULL_HANDLE;
	VkPipeline m_CausticsPipeline = VK_NULL_HANDLE;
	VkDescriptorImageInfo m_CausticsDescriptor = {};
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_CausticsSet;
	struct CausticsPushConstants
	{
		glm::vec4 Region;      // corner x, corner z, size, resolution
		glm::vec4 Grid;        // corner x, corner z, cell size, cells per side
		glm::vec4 WaveOffsets;
		glm::vec4 Waves;       // tile size 1, tile size 2, strength, unused
		glm::vec4 Sun;         // toward the sun, focus depth
		glm::vec4 Lod;         // normal map mip level of layer 1, 2
	} m_CausticsConstants = {};
	uint32_t m_CausticsGridVertices = 0;
	bool m_CausticsActive = false;
};

// The scene's water in the per-frame SceneData (see Include/WaterVolume.glslh): where it is, what it does to light and the
// caustics map's place and strength this frame (sunDirection: toward the sun). The water volume (WaterVolumeParams.y) is
// left off: the caller turns it on in the passes whose meshes are seen through the water.
void FillWaterSceneData(const EnvMapVulkanWaterSettings& settings, const glm::vec3& cameraPosition, const glm::vec3& sunDirection,
	EnvMapVulkanSceneDataGPU& data);

// Where the ray (origin + t * direction) meets the water rectangle: false when it misses it (or the water doesn't exist)
bool RaycastWater(const EnvMapVulkanWaterSettings& settings, const glm::vec3& origin, const glm::vec3& direction, float& t);
