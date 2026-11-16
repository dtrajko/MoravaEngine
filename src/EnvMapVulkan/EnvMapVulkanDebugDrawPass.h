#pragma once

#include "EnvMapVulkanDebugDraw.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>


/**
 * The renderer's side of EnvMapVulkanDebugDraw: the drawing of the lists, inside the editor overlay's render pass. Owns
 * the pipeline (made for that render pass), a host-visible buffer per frame in flight (the palette and the frame's list),
 * the persistent list's buffer (a replaced one is freed once no frame in flight can read it) and their descriptor sets.
 */
namespace EnvMapVulkanDebugDraw
{
	struct RecordInfo
	{
		VkRenderPass RenderPass = VK_NULL_HANDLE; // the overlay's: the pipeline is made for it
		VkDescriptorImageInfo SceneDepth = {};    // the G-buffer's depth, readable (EnvMapVulkanGBuffer::MakeDepthReadable)
		glm::mat4 ViewProjection = glm::mat4(1.0f); // as the scene was drawn (with TAA's jitter), so the depths match
		glm::mat4 Projection = glm::mat4(1.0f);     // its depth terms turn depths into distances
		glm::vec3 CameraPosition = glm::vec3(0.0f);
		uint32_t Width = 0, Height = 0;             // the render pass's
		uint32_t FrameSlot = 0;
	};

	// Inside the overlay's render pass (its viewport and scissor set): this frame's lists. Then the frame's list is
	// cleared, and a persistent list not set for this frame is let go.
	void Record(VkCommandBuffer commandBuffer, const RecordInfo& info);
	bool HasDrawings();
	void Shutdown();
}
