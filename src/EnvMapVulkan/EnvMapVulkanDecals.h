#pragma once

#include "EnvMapVulkanComponents.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>


class EnvMapVulkanScene;
class EnvMapVulkanGBuffer;

/**
 * The decals of the scene in the renderer (Shooter E6; World/Effects.h): the scene's Decal components (in Edit too) and the
 * decals a running game spawned (SceneEffects::Decals: bullet holes). Each frame (Update) all of them go into a buffer of the
 * frame in flight, grouped by texture; Record draws each group as one instanced draw of boxes.
 *
 * Drawn after the scene's render pass (the opaque meshes, the water, the glass) and before the particles, in a render pass of
 * their own: the scene's color, its depth read-only. A decal's box is drawn by its back faces without a depth test; each
 * pixel reads the scene's depth back to the world and, inside the box, multiplies the decal into the color (Decals.glsl).
 *
 * Not yet in: the water's reflection, render textures and the probes' capture (the main view only); motion vectors (TAA sees
 * the surface's, right for decals that stay where they were put); decals that brighten (paint on a dark wall): they darken.
 *
 * Owns: the per-frame instance buffers, the pipeline, the render pass and its framebuffer, the textures' sets.
 */
namespace EnvMapVulkanDecals
{
	void Shutdown();

	// Each frame, before the frame's commands are recorded: the decals into the frame slot's buffer
	void Update(EnvMapVulkanScene& scene, uint32_t frameSlot);

	// After the scene's render pass ended and the depth was made readable (EnvMapVulkanGBuffer::MakeDepthReadable with the
	// fragment stage among the readers): the decals multiplied into the scene's color. frameSet: set 0 (per frame);
	// viewProjection: the main view's as drawn
	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameSet, VkImage colorImage, VkFormat colorFormat, const EnvMapVulkanGBuffer& gbuffer,
		uint32_t width, uint32_t height, const glm::mat4& viewProjection, uint32_t frameSlot);

	// The scene's color and depth images were made again (the viewport's size changed)
	void OnViewportResized();

	bool HasDecals();

	struct Stats
	{
		uint32_t Placed = 0;  // the scene's Decal components
		uint32_t Spawned = 0; // the game's pool
		uint32_t Drawn = 0;   // this frame
	};
	const Stats& GetStats();
}
