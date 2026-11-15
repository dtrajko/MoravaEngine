#pragma once

#include "EnvMapVulkanComponents.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>


class EnvMapVulkanScene;
class EnvMapVulkanGBuffer;

/**
 * The particles of the scene in the renderer (ParticleEmitterComponent, World/Particles.h; docs/engine/Game_Engine_Runtime_Guide.html,
 * 12.1). The particles live on the GPU only: one pool (a storage buffer) where each emitter has a slice of its own, MaxParticles
 * long, used as a ring: a new particle takes the place of the oldest. The CPU decides each frame how many particles each emitter
 * starts and from where (Update); the GPU moves and ages them all, then starts the new ones (two compute passes), and draws
 * them (Record).
 *
 * Time: a running game's (its time scale: still while it is paused, slow in slow motion). Without a running game (the editor's
 * Edit) only the selected emitters emit, on real time: a preview. The particles alive keep moving either way.
 *
 * Drawn after the scene's render pass (the opaque meshes, the water, the glass), in a render pass of their own: the scene's
 * color, and its depth read-only (DEPTH_STENCIL_READ_ONLY_OPTIMAL: tested against, not written, and sampled for the soft
 * fade where a particle meets a surface). Each emitter is one instanced draw of its slice (squares facing the camera; a dead
 * particle's square is outside the view), the emitters far to near; the particles within an emitter aren't sorted. Lit ones
 * get the sun (through its shadow) and the ambient light; the distance haze like the meshes.
 *
 * Not yet in: the water's reflection, render textures and the probes' capture (the main view only); point and spot lights
 * on lit particles; motion vectors (TAA reprojects them as the surface behind them: right for smoke that stays where it
 * started, smeared for fast particles).
 *
 * Owns: the pool, the emitters' per-frame buffers, the compute and graphics pipelines, the render pass and its framebuffer.
 */
namespace EnvMapVulkanParticles
{
	void Shutdown();

	// Each frame, before the frame's commands are recorded. deltaTime: real seconds since the last frame; gameRunning and
	// timeScale: the game's (SetGameTime); selection: the editor's (the preview's emitters); rateScale: the quality tier's
	// share of the particles; frameSlot: the frame in flight (the emitters' buffer it writes)
	void Update(EnvMapVulkanScene& scene, float deltaTime, bool gameRunning, float timeScale, const std::vector<EnvMapVulkanEntityID>& selection,
		float rateScale, uint32_t frameSlot);

	// After the scene's render pass ended and the depth was made readable (EnvMapVulkanGBuffer::MakeDepthReadable with the
	// fragment tests among the readers): the particles moved, the new ones started, all drawn into the scene's color.
	// frameSet: set 0 (per frame); view and projection: the main view's (the projection as drawn)
	void Record(VkCommandBuffer commandBuffer, VkDescriptorSet frameSet, VkImage colorImage, VkFormat colorFormat, const EnvMapVulkanGBuffer& gbuffer,
		uint32_t width, uint32_t height, const glm::mat4& view, const glm::mat4& projection, uint32_t frameSlot);

	// The scene's color and depth images were made again (the viewport's size changed): the framebuffer and the depth's
	// descriptors that refer to them go. Their handles can't tell: a new image may get the old one's handle
	void OnViewportResized();

	bool HasEmitters();

	struct Stats
	{
		uint32_t Emitters = 0;  // in the scene
		uint32_t Emitting = 0;  // on with a rate, or bursting this frame
		uint32_t Alive = 0;     // about (from the CPU's count of those started within their lifetime)
		uint32_t Capacity = 0;  // the pool's particles
		uint32_t Started = 0;   // this frame
	};
	const Stats& GetStats();
}
