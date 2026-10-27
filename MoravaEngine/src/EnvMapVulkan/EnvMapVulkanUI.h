#pragma once

#include "H2M/Core/RefH2M.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <unordered_map>
#include <vector>


namespace H2M
{
	class FramebufferH2M;
	class PipelineH2M;
	class Texture2D_H2M;
}
struct UIDrawList;

/**
 * The game UI's GPU side (docs/engine/Game_Engine_Runtime_Guide.html, 7.2): draws a UIDrawList (UI/UILayout) into the
 * viewport composite, inside its render pass after the composite's quad: after tonemapping, so the UI's colors are the
 * colors given, and blended without changing the composite's alpha (the Viewport panel shows it with its alpha).
 *
 * The quads are written every frame into host-visible vertex and index buffers, which grow as needed (the old ones are
 * freed a few frames later: the GPU may still read them). A texture gets a descriptor set the first time it is drawn,
 * freed after it hasn't been drawn for a few frames; a quad without a texture reads a white texel.
 */
class EnvMapVulkanUI
{
public:
	// target: the framebuffer whose render pass Record is called in (the viewport composite)
	void Create(H2M::RefH2M<H2M::FramebufferH2M> target);
	void Destroy();
	bool IsValid() const { return (bool)m_Pipeline; }

	// Inside the target's render pass, with its viewport: draws the list's quads for a view of width x height pixels
	void Record(VkCommandBuffer commandBuffer, const UIDrawList& list, uint32_t width, uint32_t height);

	uint32_t GetDescriptorSetCount() const { return (uint32_t)m_TextureSets.size(); }

private:
	struct Buffer
	{
		VkBuffer Handle = VK_NULL_HANDLE;
		VkDeviceMemory Memory = VK_NULL_HANDLE;
		void* Mapped = nullptr;
		VkDeviceSize Capacity = 0;
	};
	struct TextureSet
	{
		H2M::RefH2M<H2M::Texture2D_H2M> Texture; // kept alive while its set may be read
		VkDescriptorPool Pool = VK_NULL_HANDLE;
		VkDescriptorSet Set = VK_NULL_HANDLE;
		uint64_t LastFrame = 0;
	};
	struct Retired
	{
		Buffer OldBuffer;
		VkDescriptorPool Pool = VK_NULL_HANDLE;
		uint64_t Frame = 0;
	};

	void Reserve(Buffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage);
	VkDescriptorSet GetTextureSet(const H2M::RefH2M<H2M::Texture2D_H2M>& texture);
	void FreeRetired(bool all);
	static void FreeBuffer(Buffer& buffer);

	H2M::RefH2M<H2M::PipelineH2M> m_Pipeline;
	H2M::RefH2M<H2M::Texture2D_H2M> m_White;
	Buffer m_Vertices;
	Buffer m_Indices;
	std::unordered_map<const H2M::Texture2D_H2M*, TextureSet> m_TextureSets;
	std::vector<Retired> m_Retired;
	uint64_t m_Frame = 0;
};
