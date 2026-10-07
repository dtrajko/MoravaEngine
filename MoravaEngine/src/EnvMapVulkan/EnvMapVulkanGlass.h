#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/FramebufferH2M.h"
#include "H2M/Renderer/PipelineH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>

#include <cstdint>


// The glass values of a draw: the fragment push constants of Glass_Static.glsl (block GlassMaterial, at offset 64, after
// the vertex stage's transform). Filled from the mesh's material (EnvMapVulkanMaterial, Surface = Glass).
struct GlassPushConstants
{
	glm::vec3 TintColor = glm::vec3(1.0f);
	float IOR = 1.5f;
	float Roughness = 0.0f;
	float Thickness = 0.2f;
	float AlbedoTexToggle = 0.0f;
	float NormalTexToggle = 0.0f;
	float RoughnessTexToggle = 0.0f;
	float TilingFactor = 1.0f;
	float EmissiveTexToggle = 0.0f;
	float EmissiveIntensity = 1.0f;
	float MetalRoughPacked = 0.0f;
	float SceneCopyLevels = 1.0f;
	float Solid = 1.0f;
};
static_assert(sizeof(GlassPushConstants) == 60, "GlassPushConstants must match the GlassMaterial block in Glass_Static.glsl");

/**
 * Glass (SceneEnvMapVulkan): what the renderer needs to draw meshes whose material is glass (Glass_Static.glsl).
 *
 * Glass shows what's behind it, bent and blurred, so it reads a copy of the scene: after the opaque meshes and the water
 * are drawn, the scene pass ends, its color and depth are copied (CopyScene: the color with a chain of smaller mip
 * levels, which rough glass samples for its blur), and the framebuffer's continue render pass takes over for the glass
 * (sorted back to front by the renderer) and what comes after it. A shader can't read the image it is drawing into,
 * hence the copy.
 *
 * Owns: the glass pipeline (the scene framebuffer's render pass, depth test and write, back faces culled), the copy
 * images and their samplers, and the descriptor set of the copy (set 2 of Glass_Static.glsl).
 */
class EnvMapVulkanGlass
{
public:
	static constexpr uint32_t SceneCopySet = 2;
	static constexpr uint32_t MaxCopyLevels = 8; // the blur needs about 7 (rough glass reads up to level 6)

	void Create(H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);
	void Destroy();
	// The copies follow the scene framebuffer's size (waits for the GPU: the old copies may be in use)
	void Resize(uint32_t width, uint32_t height);

	// Outside a render pass: copies the scene framebuffer's color (then fills its mip chain) and depth. The scene's
	// images are back in their render pass layouts afterwards (the continue render pass loads them).
	void CopyScene(VkCommandBuffer commandBuffer, H2M::RefH2M<H2M::FramebufferH2M> sceneFramebuffer);

	H2M::RefH2M<H2M::PipelineH2M> GetPipeline() const { return m_Pipeline; }
	VkDescriptorSet GetSceneCopySet() const { return m_DescriptorSet.DescriptorSets.empty() ? VK_NULL_HANDLE : m_DescriptorSet.DescriptorSets[0]; }
	uint32_t GetCopyLevels() const { return m_ColorLevels; }

private:
	struct CopyImage
	{
		VkImage Image = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkImageView View = VK_NULL_HANDLE;
	};
	void CreateCopyImage(CopyImage& copy, VkFormat format, VkImageAspectFlags aspect, uint32_t levels);
	void DestroyCopyImages();
	void WriteDescriptors();

	H2M::RefH2M<H2M::PipelineH2M> m_Pipeline;
	H2M::VulkanShaderH2M::ShaderMaterialDescriptorSet m_DescriptorSet;
	CopyImage m_Color, m_Depth;
	uint32_t m_Width = 0, m_Height = 0;
	uint32_t m_ColorLevels = 1;
	VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED, m_DepthFormat = VK_FORMAT_UNDEFINED;
	VkSampler m_ColorSampler = VK_NULL_HANDLE; // trilinear (the blur blends between levels)
	VkSampler m_DepthSampler = VK_NULL_HANDLE; // nearest (depth values are not filtered)
};
