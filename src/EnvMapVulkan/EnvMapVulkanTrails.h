#pragma once

#include "EnvMapVulkanComponents.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>


class EnvMapVulkanScene;

/**
 * The trails of the scene in the renderer (TrailComponent, World/Trails.h; docs/engine/Game_Engine_Runtime_Guide.html, 12.2):
 * skid marks. Each frame (Update) a trail that moved far enough leaves a point in its component's ring; the points of all
 * trails, and a last one where each emitting entity is now (the ribbon reaches the wheel, not the point before it), go into
 * a buffer of the frame in flight, each with its ribbon's sideways half width and its color (faded by its age and, in a full
 * ring, among the oldest).
 *
 * Time: a running game's (still while paused); without one (the editor's Edit) only the selected trails leave points, on
 * real time: a preview. The points belong to the scene: a copy of it (Play) starts without any.
 *
 * Drawn inside the scene's render pass, right after the opaque meshes (before the water and the glass: those see the marks
 * through them): one draw per trail, two triangles per piece of ribbon, made in the vertex stage from the points (no vertex
 * buffer). Blended by multiplying: the mark darkens the lit surface under it as a darker surface would (its light, shadows
 * and reflections stay), faded by the distance haze. Tested against the scene's depth, not written, lifted a little along
 * the entity's up and biased toward the camera so it doesn't flicker on the road.
 *
 * Not yet in: the water's reflection, render textures and the probes' capture (the main view only); marks lighter than the
 * surface (snow, chalk); culling (a trail is drawn whole wherever it is).
 */
namespace EnvMapVulkanTrails
{
	void Shutdown();

	// Each frame, before the frame's commands are recorded. deltaTime: real seconds since the last frame; gameRunning and
	// timeScale: the game's (SetGameTime); selection: the editor's (the preview's trails); frameSlot: the frame in flight
	void Update(EnvMapVulkanScene& scene, float deltaTime, bool gameRunning, float timeScale, const std::vector<EnvMapVulkanEntityID>& selection,
		uint32_t frameSlot);

	// Inside the scene's render pass (renderPass: its own, the pipeline is made for it), after the opaque meshes: the marks,
	// over them. frameSet: set 0 (per frame)
	void Record(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkDescriptorSet frameSet, uint32_t frameSlot);

	bool HasPoints();

	struct Stats
	{
		uint32_t Trails = 0;   // in the scene
		uint32_t Emitting = 0; // leaving points this frame
		uint32_t Points = 0;   // drawn
	};
	const Stats& GetStats();
}
