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

	// Look
	glm::vec3 ScatterColor = glm::vec3(0.012f, 0.045f, 0.055f); // light the water body sends back up (linear)
	float Roughness = 0.06f;            // the sun highlight's size and the reflection's blur
	float ReflectionStrength = 1.0f;    // 1 = physically based

	// Seeing into the water (refraction): light is absorbed along its path through the water, red first (Beer-Lambert)
	glm::vec3 Transmittance = glm::vec3(0.55f, 0.85f, 0.88f); // the part of each color that is left after Clarity meters
	float Clarity = 1.5f;               // meters (larger: clearer water, the bottom stays visible deeper)
	float RefractionStrength = 1.0f;    // how much the waves bend the view into the water
	float EdgeSoftness = 0.3f;          // meters of water over which the surface fades in at the shore
	float FoamAmount = 0.6f;            // foam along the shore and around objects (0: none)
	float FoamWidth = 0.4f;             // meters of water depth that get foam

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
 */
class EnvMapVulkanWater
{
public:
	// targetFramebuffer: the HDR scene framebuffer the water is drawn into (after the opaque meshes)
	void Create(H2M::RefH2M<H2M::FramebufferH2M> targetFramebuffer);
	void Destroy();
	bool IsValid() const { return (bool)m_Pipeline; }

	// Recreates the scene copies for a new size of the scene framebuffer (call after resizing it)
	void Resize(uint32_t width, uint32_t height);

	// Moves the waves and writes the settings for this frame; projection: the camera's (to turn depth into distance)
	void Update(const EnvMapVulkanWaterSettings& settings, float deltaTime, const glm::mat4& projection);
	// Outside a render pass: copies the scene framebuffer's color and depth into the textures the water samples, and leaves
	// the framebuffer's attachments in the layouts its continue render pass expects
	void CopyScene(VkCommandBuffer commandBuffer, H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	// Draws the water inside the scene's render pass; frameDescriptorSet is the per-frame set (set 0)
	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameDescriptorSet, const EnvMapVulkanWaterSettings& settings);

private:
	H2M::RefH2M<H2M::PipelineH2M> m_Pipeline;
	H2M::RefH2M<H2M::VertexBufferH2M> m_VertexBuffer;
	H2M::RefH2M<H2M::IndexBufferH2M> m_IndexBuffer;
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
};

// Where the ray (origin + t * direction) meets the water rectangle: false when it misses it (or the water doesn't exist)
bool RaycastWater(const EnvMapVulkanWaterSettings& settings, const glm::vec3& origin, const glm::vec3& direction, float& t);
