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
 * Impostors (Phase 8.4): a species with one (FoliageSpecies::Impostor: a model of one card with atlases of 16 views, baked by
 * Tools/TrackBuilder/bake_impostors.py) is drawn as it beyond its ImpostorDistance from the camera: each far copy is its card
 * showing the view nearest to the camera's direction in the copy's own space (picked by the shaders: all of a species' cards
 * are one draw), turned about its vertical axis to face the camera,
 * at the copy's place and size. Between ImpostorDistance and ImpostorDistance + ImpostorFade a copy is drawn both ways with
 * complementary dithers (the fade rides in the transform's bottom row: Include/MeshInstance.glslh). So the main view's
 * copies change with the camera: CullMainView writes them each frame into a buffer of their own (host-visible: one frame
 * in flight), the static buffer stays the shadow maps' (a far copy's shadow is still its model's).
 *
 * Not yet in: the water's reflection, render textures, the probes' capture, ray tracing (see the engine guide's foliage section).
 *
 * Owns: the instance buffer (all copies, for the shadow maps) and the main view's (this frame's models and cards).
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
		const H2M::ModelH2M* Model = nullptr; // a species' model, or its impostor's (a view's card)
		uint32_t Mesh = 0;
		EnvMapVulkanMaterial* Material = nullptr;
		uint32_t FirstInstance = 0, InstanceCount = 0;
	};

	// How the main view draws a species with an impostor (the view option Foliage LOD)
	enum class LodMode { Distance, ModelsOnly, ImpostorsOnly };

	// The main view (the prepass and the scene pass draw the same): the cells in the frustum and within the draw distance
	// of viewPosition, each copy as its model or its impostor. culling: false draws every cell (Frustum Culling off). Each
	// frame, before those passes (it writes the main view's instance buffer)
	// tint: the cards marked for the scene pass to tint (the debug view Show Impostors)
	void CullMainView(const glm::vec4 planes[6], const glm::vec3& viewPosition, bool culling, LodMode lod = LodMode::Distance, bool tint = false);
	const std::vector<Draw>& GetMainViewDraws();
	VkBuffer GetMainViewInstanceBuffer(); // the main view's draws' instances (VK_NULL_HANDLE: none this frame)
	// A shadow map's view: the cells in its frustum within the shadow distance of the camera (and within lightRadius of
	// lightCenter, a spot or point light's)
	void CollectShadowDraws(const glm::vec4 planes[6], const glm::vec3& viewPosition, std::vector<Draw>& draws, const glm::vec3* lightCenter = nullptr,
		float lightRadius = 0.0f);

	VkBuffer GetInstanceBuffer(); // every copy as its model, for the shadow maps (VK_NULL_HANDLE: no copies)
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
		uint32_t Copies = 0, CopiesDrawn = 0; // the main view's: CopiesDrawn as models
		uint32_t ImpostorsDrawn = 0, Fading = 0; // as impostors; of them, cross-fading (drawn both ways)
		uint32_t Cells = 0, CellsDrawn = 0;
		uint32_t Species = 0;
	};
	const Stats& GetStats();
}
