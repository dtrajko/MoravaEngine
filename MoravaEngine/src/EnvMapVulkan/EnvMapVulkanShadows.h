#pragma once

#include <glm/glm.hpp>
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include <array>
#include <cstdint>


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
 * The shadow map's GPU resources: one 32-bit float depth image with a layer per cascade.
 * - rendering: a depth-only render pass and a framebuffer per layer
 * - sampling: an array view of all layers with a comparison sampler (sampler2DArrayShadow), and a plain sampler for
 *   showing the layers in the UI
 * Between shadow passes the image is in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (also right after Create).
 */
class EnvMapVulkanShadowMap
{
public:
	// (Re)creates the resources. Nothing in flight may still use the old ones.
	void Create(uint32_t resolution);
	// Called explicitly (renderer Shutdown), not by a destructor: a static shadow map would be destroyed after the device
	void Destroy();

	bool IsValid() const { return m_Image != VK_NULL_HANDLE; }
	uint32_t GetResolution() const { return m_Resolution; }
	VkRenderPass GetRenderPass() const { return m_RenderPass; }
	VkFramebuffer GetFramebuffer(uint32_t cascade) const { return m_Framebuffers[cascade]; }
	VkImageView GetArrayView() const { return m_ArrayView; }
	VkImageView GetLayerView(uint32_t cascade) const { return m_LayerViews[cascade]; }
	VkImageView GetDisplayView(uint32_t cascade) const { return m_DisplayViews[cascade]; } // depth shown as gray (r, r, r)
	VkSampler GetCompareSampler() const { return m_CompareSampler; }
	VkSampler GetDisplaySampler() const { return m_DisplaySampler; }

	static constexpr VkFormat Format = VK_FORMAT_D32_SFLOAT;

private:
	uint32_t m_Resolution = 0;
	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_Allocation = nullptr;
	VkImageView m_ArrayView = VK_NULL_HANDLE;
	std::array<VkImageView, ShadowCascadeCount> m_LayerViews = {};
	std::array<VkImageView, ShadowCascadeCount> m_DisplayViews = {};
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	std::array<VkFramebuffer, ShadowCascadeCount> m_Framebuffers = {};
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
