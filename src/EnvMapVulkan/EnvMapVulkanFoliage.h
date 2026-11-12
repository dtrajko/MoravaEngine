#pragma once

#include "EnvMapVulkanComponents.h"

#include "H2M/Core/RefH2M.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>


class EnvMapVulkanScene;
class EnvMapVulkanMaterial;
namespace H2M
{
	class ModelH2M;
}

/**
 * The foliage of the scene in the renderer (FoliageComponent, World/Foliage.h; the racing game's Phase 8). Update, at the start
 * of each frame, scatters what a component's settings ask for when they or its terrain changed (the keep-out asks a physics
 * world of the scene's static colliders: ScenePhysics::StartStatic), loads each species' model (ModelH2M::LoadShared, its own
 * materials from the Material Library) and writes every copy's transform into one instance buffer, species by species and
 * cell by cell.
 *
 * The copies are drawn instanced: one draw per mesh of a species for each run of neighboring cells in view, the transforms
 * read per instance (binding 1, locations 7 to 10: Include/MeshInstance.glslh) by the instanced variants of the passes'
 * shaders (HazelPBR_Instanced.glsl, GBufferPrepass_Instanced.glsl and its cutout, ShadowDepth_Instanced.glsl and its cutout).
 * Cells are culled against the view's frustum, and beyond DrawDistance (ShadowDistance for the shadow maps) from the camera.
 *
 * Not yet in: the water's reflection, render textures, the probes' capture, ray tracing (see the engine guide's foliage section).
 *
 * Owns: the instance buffer.
 */
namespace EnvMapVulkanFoliage
{
	// Before any command buffer of the frame is recorded (a new instance buffer waits for the GPU before the old one goes)
	void Update(EnvMapVulkanScene& scene);
	void Shutdown();
	// (A component without copies is scattered again: the Inspector's Scatter Again resets FoliageComponent::Instances)

	// One instanced draw: a mesh of a species' model, for a range of its copies in the instance buffer
	struct Draw
	{
		const H2M::ModelH2M* Model = nullptr;
		uint32_t Mesh = 0;
		EnvMapVulkanMaterial* Material = nullptr;
		uint32_t FirstInstance = 0, InstanceCount = 0;
	};

	// The main view (the prepass and the scene pass draw the same): the cells in the frustum and within the draw distance
	// of viewPosition. culling: false draws every cell (Frustum Culling off). Each frame, before those passes
	void CullMainView(const glm::vec4 planes[6], const glm::vec3& viewPosition, bool culling);
	const std::vector<Draw>& GetMainViewDraws();
	// A shadow map's view: the cells in its frustum within the shadow distance of the camera (and within lightRadius of
	// lightCenter, a spot or point light's)
	void CollectShadowDraws(const glm::vec4 planes[6], const glm::vec3& viewPosition, std::vector<Draw>& draws, const glm::vec3* lightCenter = nullptr,
		float lightRadius = 0.0f);

	VkBuffer GetInstanceBuffer(); // VK_NULL_HANDLE: no copies
	bool HasCopies();

	// The cells for the debug view Show Foliage Cells: each one's box in the world, and whether the main view draws it
	struct CellBox
	{
		glm::vec3 Min = glm::vec3(0.0f), Max = glm::vec3(0.0f);
		bool Drawn = false;
	};
	const std::vector<CellBox>& GetCellBoxes();

	struct Stats
	{
		uint32_t Copies = 0, CopiesDrawn = 0; // the main view's
		uint32_t Cells = 0, CellsDrawn = 0;
		uint32_t Species = 0;
	};
	const Stats& GetStats();
}
