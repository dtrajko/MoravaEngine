#pragma once

#include <glm/glm.hpp>
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include <array>
#include <cstdint>
#include <vector>


/**
 * Cascaded shadow maps for the sun (SceneEnvMapVulkan). The camera's view, from its near plane to Distance, is cut into
 * ShadowCascadeCount slices; each slice gets its own orthographic shadow map seen from the sun, all of them layers of one
 * depth image. Near slices are short (sharp shadows close to the camera), far slices long.
 */
constexpr uint32_t ShadowCascadeCount = 4;

struct EnvMapVulkanShadowSettings
{
	float Distance = 40.0f;      // shadows end (fade out) at this distance from the camera
	uint32_t Resolution = 2048;  // of each cascade (1024, 2048 or 4096)
	float SplitLambda = 0.75f;   // 0: equal slices, 1: logarithmic slices (more resolution near the camera)
	float Softness = 1.0f;       // PCF filter spacing in texels (0: the hardware 2x2 filter only)
	float DepthBias = 1.25f;     // constant depth bias of the shadow pass (Vulkan depth bias units)
	float SlopeBias = 1.75f;     // depth bias that grows with the surface slope, seen from the sun
	float NormalBias = 1.0f;     // receivers look the shadow map up this many texels off their surface, along the normal
	bool ShowCascades = false;   // tints the scene by cascade (red, green, blue, yellow)
};

struct EnvMapVulkanShadowCascade
{
	glm::mat4 ViewProjection = glm::mat4(1.0f); // world -> shadow map clip space (Vulkan: z 0..1)
	float SplitDistance = 0.0f;                 // distance along the camera's view direction where the cascade ends
	float TexelWorldSize = 0.0f;                // world size of one shadow map texel (for the normal bias)
};

/**
 * The cascades for the current camera, in world space:
 * - frustumCornerRays: unit directions from the camera through the 4 corners of the view (any order)
 * - towardsLight: unit vector from the scene towards the sun
 * - sceneBoundsMin/Max: all shadow casters, so a caster outside the camera's view still casts into it
 * The slices overlap by a tenth of the previous slice, the blend zone between cascades.
 * Stable: a cascade's size doesn't change when the camera turns (it is fitted to a sphere around its slice), and its
 * position snaps to whole shadow map texels when the camera moves, so shadow edges don't shimmer.
 */
void ComputeShadowCascades(const glm::vec3& cameraPosition, const glm::vec3& cameraForward, const std::array<glm::vec3, 4>& frustumCornerRays,
	float nearDistance, const EnvMapVulkanShadowSettings& settings, const glm::vec3& towardsLight,
	const glm::vec3& sceneBoundsMin, const glm::vec3& sceneBoundsMax, std::array<EnvMapVulkanShadowCascade, ShadowCascadeCount>& cascades);

/**
 * Shadows of the spot and point lights. Up to MaxShadowedSpotLights spot lights and MaxShadowedPointLights point lights
 * cast shadows at once (each light has a Cast Shadows option); each one gets a slot in a shadow map array.
 * - spot light: one perspective shadow map looking down the spot, a layer of a 2D array
 * - point light: a cube of 6 perspective shadow maps (90 degrees each), 6 layers of a cube map array
 */
constexpr uint32_t MaxShadowedSpotLights = 4;
constexpr uint32_t MaxShadowedPointLights = 4;

struct EnvMapVulkanLocalShadowSettings
{
	uint32_t SpotResolution = 1024; // of each spot light's map (512, 1024 or 2048)
	uint32_t PointResolution = 512; // of each cube face (256, 512 or 1024)
	float Softness = 1.0f;          // PCF filter spacing in texels
	float DepthBias = 1.25f;        // as EnvMapVulkanShadowSettings
	float SlopeBias = 1.75f;
	float NormalBias = 1.0f;
};

// Near plane of a local light's shadow maps: a small fraction of the range (the depth precision is spent near it)
float GetLocalShadowNearPlane(float range);

// A spot light's shadow map: world -> clip space (Vulkan: z 0..1), seen from the light down its direction, with a field
// of view a little wider than the outer cone (so the PCF filter at the edge of the cone stays inside the map)
glm::mat4 ComputeSpotShadowViewProjection(const glm::vec3& position, const glm::vec3& direction, float outerAngle, float range);
// That field of view (degrees): the cone's full angle plus 5 degrees, at most 170
float GetSpotShadowFieldOfView(float outerAngle);

// The depth comparison parameters of a point light's cube faces: depth = x - y / z, z the largest coordinate of the
// offset from the light (as PointShadowDepth, for the shaders)
glm::vec2 GetPointShadowDepthParams(float range);

/**
 * A point light's 6 cube faces: world -> clip space (Vulkan: z 0..1), in the layer order of a Vulkan cube map
 * (+X, -X, +Y, -Y, +Z, -Z). Each face is oriented as Vulkan's cube map lookup expects (the face's s, t coordinates of a
 * direction, see "Cube Map Face Selection" in the Vulkan spec), so a samplerCube lookup with the direction from the light
 * to a point reads the texel the point was rendered to. The depth stored for a point is the perspective depth of its
 * largest coordinate (relative to the light): see PointShadowDepth.
 */
std::array<glm::mat4, 6> ComputePointShadowFaceViewProjections(const glm::vec3& position, float range);
// The same 6 faces with any near and far plane: a cube map drawn around a point (the probes' capture, see EnvMapVulkanProbes)
std::array<glm::mat4, 6> ComputeCubeFaceViewProjections(const glm::vec3& position, float nearPlane, float farPlane);

// The depth a point at offset (point - light position) has in its cube face: what the shader compares against
float PointShadowDepth(const glm::vec3& offset, float range);

/**
 * A depth image with a layer per shadow map (sun cascades, spot lights) or 6 layers per cube (point lights), 32-bit float.
 * - rendering: a depth-only render pass and a framebuffer per layer
 * - sampling: a view of all layers (2D array: sampler2DArrayShadow; cube array: samplerCubeArrayShadow) with a comparison
 *   sampler, and a plain sampler for showing the layers in the UI
 * Between shadow passes the image is in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (also right after Create).
 */
class EnvMapVulkanShadowMap
{
public:
	// (Re)creates the resources. layerCount: a multiple of 6 for a cube array (needs the imageCubeArray device feature).
	// Nothing in flight may still use the old ones.
	void Create(uint32_t resolution, uint32_t layerCount, bool cubeArray = false);
	// Called explicitly (renderer Shutdown), not by a destructor: a static shadow map would be destroyed after the device
	void Destroy();

	bool IsValid() const { return m_Image != VK_NULL_HANDLE; }
	// Increases with every Create: tells users of the views (the UI) to refresh them. Not the view handles: Vulkan may
	// give a new view the handle of a destroyed one.
	uint32_t GetGeneration() const { return m_Generation; }
	uint32_t GetResolution() const { return m_Resolution; }
	uint32_t GetLayerCount() const { return m_LayerCount; }
	bool IsCubeArray() const { return m_CubeArray; }
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkFramebuffer GetFramebuffer(uint32_t layer) const { return m_Framebuffers[layer]; }
	VkImageView GetArrayView() const { return m_ArrayView; } // all layers: 2D array or cube array
	VkImageView GetFlatArrayView() const { return m_FlatArrayView; } // all layers as a 2D array (also a cube array's faces)
	VkImageView GetLayerView(uint32_t layer) const { return m_LayerViews[layer]; }
	VkImageView GetDisplayView(uint32_t layer) const { return m_DisplayViews[layer]; } // depth shown as gray (r, r, r)
	VkSampler GetCompareSampler() const { return m_CompareSampler; }
	VkSampler GetDisplaySampler() const { return m_DisplaySampler; }

	static constexpr VkFormat Format = VK_FORMAT_D32_SFLOAT;

private:
	uint32_t m_Resolution = 0;
	uint32_t m_LayerCount = 0;
	bool m_CubeArray = false;
	uint32_t m_Generation = 0;
	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = nullptr;
	VkImageView m_ArrayView = VK_NULL_HANDLE;
	VkImageView m_FlatArrayView = VK_NULL_HANDLE;
	std::vector<VkImageView> m_LayerViews;
	std::vector<VkImageView> m_DisplayViews;
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	std::vector<VkFramebuffer> m_Framebuffers;
	VkSampler m_CompareSampler = VK_NULL_HANDLE;
	VkSampler m_DisplaySampler = VK_NULL_HANDLE;
};

/**
 * A depth-only pipeline for the shadow pass (ShadowDepth.glsl, ShadowDepth_Anim.glsl), built directly for the shadow map's
 * render pass. No culling (thin and open models cast shadows too); depth bias is dynamic (vkCmdSetDepthBias), so the
 * settings apply immediately.
 */
struct EnvMapVulkanShadowPipeline
{
	VkPipeline Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout Layout = VK_NULL_HANDLE;

	// usedLocations: the vertex attributes the shader reads (only these are described, the others are skipped)
	void Create(H2M::RefH2M<H2M::VulkanShaderH2M> shader, const H2M::VertexBufferLayoutH2M& vertexLayout, const std::vector<uint32_t>& usedLocations,
		VkRenderPass renderPass);
	void Destroy();
};

/**
 * Draws a spot light's shadow map, or a point light's cube unfolded into a cross, into a color image for the UI
 * (ShadowMapView.glsl), with the depth converted back to the distance from the light so the image is readable. Recorded
 * into the frame's command buffer after the shadow passes, only while a viewer is shown.
 */
class EnvMapVulkanShadowMapViewer
{
public:
	static constexpr uint32_t Width = 1024; // 4:3, the cross is 4 x 3 tiles of 256
	static constexpr uint32_t Height = 768;

	void Create(H2M::RefH2M<H2M::VulkanShaderH2M> shader);
	void Destroy();
	bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

	// cube: the 6 faces from baseLayer as a cross; otherwise layer baseLayer as a square in the left Height x Height
	void Record(VkCommandBuffer commandBuffer, const EnvMapVulkanShadowMap& shadowMap, bool cube, uint32_t baseLayer, float nearPlane, float farPlane);

	VkImageView GetView() const { return m_View; }
	VkSampler GetSampler() const { return m_Sampler; }

private:
	H2M::RefH2M<H2M::VulkanShaderH2M> m_Shader;
	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = nullptr;
	VkImageView m_View = VK_NULL_HANDLE;
	VkSampler m_Sampler = VK_NULL_HANDLE;
	VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;
	const EnvMapVulkanShadowMap* m_BoundMap = nullptr; // the shadow map the descriptor set points to
	uint32_t m_BoundGeneration = 0;
};
