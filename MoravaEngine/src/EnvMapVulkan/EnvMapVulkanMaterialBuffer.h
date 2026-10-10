#pragma once

#include "EnvMapVulkanMaterialLibrary.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * The materials of the Material Library in one GPU buffer (SceneEnvMapVulkan), one entry per material in the library's
 * order (Phase B of docs/rendering/SSAO_GI_RayTracing_Guide.html). A draw knows its material through the material's
 * descriptor set (set 1: the maps) and push constants (the values); a shader that lights a point it didn't draw, such as
 * what a probe sees or what a ray hits, has neither. It reads the material here, by index.
 *
 * An entry holds plain values only. Where a material uses a map, the entry has the map's average color: enough for light
 * that bounces, which is blurred over the whole scene (see ShadeSurfaceDiffuse in Include/ShadeSurface.glslh). The fields
 * for the maps themselves (indices into an array of all textures) are there for ray tracing and not used yet.
 *
 * Update gives every material its index (the "MaterialIndex" value, pushed with the material's other values) and uploads
 * the entries when any of them changed. Shaders: u_Materials in Include/FrameSet.glslh (set 0, binding 17).
 *
 * Owns: the buffer (a storage buffer, host visible) and the cache of the maps' average colors.
 */
class EnvMapVulkanMaterialBuffer
{
public:
	// std430, 64 bytes: must match GPUMaterial in Include/FrameSet.glslh
	struct GPUMaterial
	{
		glm::vec4 AlbedoColor = glm::vec4(0.0f); // rgb: the albedo color, or the average color of the albedo map when it is used
		glm::vec4 Emissive = glm::vec4(0.0f);    // rgb: the average color of the emissive map * the emissive intensity
		float Metalness = 0.0f;
		float Roughness = 0.0f;
		uint32_t Flags = 0;
		uint32_t AlbedoMapIndex = NoMap;
		uint32_t NormalMapIndex = NoMap;
		uint32_t RoughnessMapIndex = NoMap;
		uint32_t MetalnessMapIndex = NoMap;
		uint32_t EmissiveMapIndex = NoMap;
	};
	static constexpr uint32_t NoMap = 0xFFFFFFFF;
	static constexpr uint32_t FlagGlass = 1u << 0;

	void Create();
	void Destroy();

	// Call once per frame, after the materials were edited and before anything is recorded. Returns true when the buffer
	// was replaced by a bigger one (it waited for the GPU): descriptor sets that point to it must be written again.
	bool Update();

	VkDescriptorBufferInfo GetDescriptorInfo() const { return { m_Buffer, 0, VK_WHOLE_SIZE }; }
	// What was uploaded last, by material index (for the editor)
	const std::vector<GPUMaterial>& GetEntries() const { return m_Entries; }

private:
	void CreateBuffer(uint32_t capacity);
	void DestroyBuffer();
	GPUMaterial Pack(const H2M::RefH2M<EnvMapVulkanMaterial>& material);
	// The average of the map's pixels (color maps: of their linear colors), computed once per image file
	glm::vec4 GetAverageColor(const H2M::RefH2M<H2M::Texture2D_H2M>& texture, bool srgb);

	VkBuffer m_Buffer = VK_NULL_HANDLE;
	VkDeviceMemory m_Memory = VK_NULL_HANDLE;
	uint32_t m_Capacity = 0; // entries
	std::vector<GPUMaterial> m_Entries;
	std::unordered_map<std::string, glm::vec4> m_AverageColors;
};
