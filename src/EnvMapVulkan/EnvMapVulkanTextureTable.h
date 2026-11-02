#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Platform/Vulkan/VulkanShaderH2M.h"
#include "H2M/Renderer/TextureH2M.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <unordered_map>
#include <vector>


/**
 * Which element of the array of all textures each texture has (SceneEnvMapVulkan, Phase G of
 * docs/rendering/SSAO_GI_RayTracing_Guide.html). A mesh shader gets its material's maps through the material's descriptor
 * set; a ray can hit any material, so its shader has one big array, "sampler2D u_Textures[]" in the ray tracing set
 * (Include/RayTracing.glslh), and the material buffer's entries say which element each map is.
 *
 * The table only keeps the numbers. A texture gets an element when the material buffer first asks for it (GetSlot) and
 * keeps it while any material uses it: the material buffer brackets its pass over the materials with BeginUse / EndUse,
 * and an element nobody asked for in between is free again. New elements wait in the writes (TakeWrites) for the owner
 * of the descriptor set to write them (EnvMapVulkanRayTracing).
 *
 * Owns: a reference to every texture in the table.
 */
class EnvMapVulkanTextureTable
{
public:
	static constexpr uint32_t Capacity = H2M::VulkanShaderH2M::BindlessArraySize;
	static constexpr uint32_t NoSlot = 0xFFFFFFFF;

	struct Write
	{
		uint32_t Slot = 0;
		VkDescriptorImageInfo Info = {};
	};

	void BeginUse();
	// The texture's element, a new one when it has none. NoSlot: the table is full (the caller's value takes over).
	uint32_t GetSlot(const H2M::RefH2M<H2M::Texture2D_H2M>& texture);
	void EndUse();

	// The elements given out since the last call, with what to write into them
	std::vector<Write> TakeWrites();

	uint32_t GetCount() const { return (uint32_t)m_Index.size(); } // textures in the table
	void Clear();

private:
	struct Slot
	{
		H2M::RefH2M<H2M::Texture2D_H2M> Texture;
		bool Used = false;
	};
	std::vector<Slot> m_Slots;
	std::vector<uint32_t> m_FreeSlots;
	std::unordered_map<const H2M::Texture2D_H2M*, uint32_t> m_Index;
	std::vector<Write> m_Writes;
};
