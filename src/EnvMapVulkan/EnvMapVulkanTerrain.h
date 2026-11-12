#pragma once

#include "EnvMapVulkanComponents.h"

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <cstdint>


class EnvMapVulkanScene;
namespace H2M
{
	class ModelH2M;
}

/**
 * The terrains of the scene in the renderer (SceneEnvMapVulkan; TerrainComponent, World/Terrain.h). Update, at the start of
 * each frame, makes what a terrain's settings ask for: its heights and its chunks (a ModelH2M made in memory) when the shape
 * changed, and its layers' materials found by ID. The chunks are then drawn as any model's meshes (EnvMapVulkanSceneData
 * lists the terrains with the models): the shadows, the culling, the ray tracing and the probes take them as they are.
 *
 * What differs is the look. The passes that shade a mesh's surface (the G-buffer prepass, the scene pass, the water's
 * reflection, the render textures) draw a terrain with the terrain variants of the PBR shaders (HazelPBR_Terrain.glsl,
 * GBufferPrepass_Terrain.glsl: Include/TerrainMaterial.glslh), which mix its three layers by height and slope. Their set 1
 * is a material set (the same layout, so the PBR pipelines' layouts fit) holding the layers' maps: the albedo and normal
 * map of the sand, the grass and the rock. Their push constants are the terrain's values (GetUniforms) in place of a
 * material's. The rays and the probes, which read materials from the material buffer, see the grass layer.
 *
 * Owns: each terrain's descriptor set (by entity).
 */
namespace EnvMapVulkanTerrain
{
	// The terrains' heights, chunks and layer materials, as their settings ask. Before any command buffer of the frame is
	// recorded (a changed map waits for the GPU before the set is written)
	void Update(EnvMapVulkanScene& scene);
	void Shutdown();

	// The guides of a terrain (TerrainComponent::Guides): what they are now, cheaply (each frame: their models and where they
	// are from the terrain, not their triangles; 0: none), and their triangles in the terrain's space
	uint64_t GetGuidesHash(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const TerrainComponent& terrain);
	TerrainGuides GatherGuides(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const TerrainComponent& terrain);
	// The heights its settings and guides give, made again when either changed (the renderer, physics: a game can start
	// before the first frame is drawn). True: made now
	bool UpdateHeights(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, TerrainComponent& terrain);

	// The fragment push constants of the terrain shaders: must match the block in Include/TerrainMaterial.glslh (64 bytes,
	// at offset 64, where a material's values are)
	struct Uniforms
	{
		float LayerMaterial[3] = { -1.0f, -1.0f, -1.0f }; // each layer's entry in the material buffer (-1: none, a plain color)
		float MaterialIndex = 0.0f;                        // the entry the material functions report (the grass's)
		float SandHeight = 0.0f;                           // world heights
		float RockHeight = 0.0f;
		float RockSlopeStart = 0.0f;                       // 1 - the normal's y where rock starts, and where it is all rock
		float RockSlopeEnd = 0.0f;
		float MapToggles = 0.0f;                           // bit 2 * layer: its albedo map, bit 2 * layer + 1: its normal map
		float LodTint = -1.0f;                             // the debug view Show Terrain LOD: the chunk's level (-1: off)
		float Padding[6] = {};
	};
	static_assert(sizeof(Uniforms) == 64, "the terrain block of Include/TerrainMaterial.glslh is 64 bytes");

	// The terrain a model is (the chunks of one): its set and values for this frame. False: the model isn't a terrain's
	bool Find(const H2M::ModelH2M* model, VkDescriptorSet& descriptorSet, Uniforms& uniforms);

	// Levels of detail (TerrainLods): a chunk farther from the camera is drawn with fewer of its heights. The levels are
	// chosen from the main view's camera in every pass (the shadow maps too), so the passes see the same ground
	void SetViewPosition(const glm::vec3& worldPosition); // each frame, before the passes are recorded

	// What a terrain's chunk draws: its level's triangles and skirt
	struct ChunkDraw
	{
		TerrainLods::Range Surface, Skirt;
		uint32_t Level = 0;
	};
	// False: the model isn't a terrain's with levels (draw the mesh as it is). meshTransform: the chunk's, in the world.
	// viewPosition: where the level is chosen from instead of the camera (a probe's capture), toleranceScale: the error
	// allowed, times the terrain's own (a probe's small cube faces show much less)
	bool ChooseChunk(const H2M::ModelH2M* model, size_t meshIndex, const glm::mat4& meshTransform, ChunkDraw& draw,
		const glm::vec3* viewPosition = nullptr, float toleranceScale = 1.0f);
}
