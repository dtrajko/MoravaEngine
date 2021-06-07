#pragma once


// #define SCENE_COTTAGE
// #define SCENE_EIFFEL
// #define SCENE_SPONZA
// #define SCENE_TERRAIN
// #define SCENE_PBR
// #define SCENE_LEARN_OPENGL
// #define SCENE_BULLET
// #define SCENE_INSTANCED
// #define SCENE_ASTEROIDS
// #define SCENE_NANOSUIT
// #define SCENE_FRAMEBUFFERS
// #define SCENE_CUBEMAPS
// #define SCENE_PARTICLES
// #define SCENE_OMNI_SHADOWS
// #define SCENE_VOXEL_TERRAIN
// #define SCENE_PROCEDURAL_LANDMASS
// #define SCENE_VOXEL_TERRAIN_SL
// #define SCENE_MARCHING_CUBES
// #define SCENE_SSAO
// #define SCENE_BLOOM
// #define SCENE_DEFERRED
// #define SCENE_DEFERRED_OGL
// #define SCENE_EDITOR
// #define SCENE_EDITOR_IMGUIZMO
// #define SCENE_ANIM_PBR
#define SCENE_HAZEL_ENV_MAP
// #define SCENE_HAZEL_VULKAN
// #define SCENE_DX11


enum class SceneName
{
	NONE = 0,
	COTTAGE,
	EIFFEL,
	SPONZA,
	TERRAIN,
	PBR,
	LEARN_OPENGL,
	BULLET,
	INSTANCED,
	ASTEROIDS,
	NANOSUIT,
	FRAMEBUFFERS,
	CUBEMAPS,
	PARTICLES,
	OMNI_SHADOWS,
	VOXEL_TERRAIN,
	PROCEDURAL_LANDMASS,
	VOXEL_TERRAIN_SL,
	MARCHING_CUBES,
	SSAO,
	BLOOM,
	DEFERRED,
	DEFERRED_OGL,
	EDITOR,
	EDITOR_IMGUIZMO,
	ANIM_PBR,
	HAZEL_ENV_MAP,
	HAZEL_VULKAN,
	DX11,
};


class Scene;
class RendererBasic;

struct SceneProperties
{
	Scene* Scene;
	RendererBasic* Renderer;
	SceneName Name;
	// Hazel::RendererAPIType RendererAPI;
};


#if defined(SCENE_COTTAGE)
#include "Scene/SceneCottage.h"
#include "Renderer/Renderer.h"
#elif defined(SCENE_EIFFEL)
#include "Scene/SceneEiffel.h"
#include "Renderer/Renderer.h"
#elif defined(SCENE_SPONZA)
#include "Scene/SceneSponza.h"
#include "Renderer/Renderer.h"
#elif defined(SCENE_TERRAIN)
#include "Scene/SceneTerrain.h"
#include "Renderer/Renderer.h"
#elif defined(SCENE_PBR)
#include "Scene/ScenePBR.h"
#include "Renderer/RendererPBR.h"
#elif defined(SCENE_LEARN_OPENGL)
#include "Scene/SceneJoey.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_BULLET)
#include "Scene/SceneBullet.h"
#include "Renderer/Renderer.h"
#elif defined(SCENE_INSTANCED)
#include "Scene/SceneInstanced.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_ASTEROIDS)
#include "Scene/SceneAsteroids.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_NANOSUIT)
#include "Scene/SceneNanosuit.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_FRAMEBUFFERS)
#include "Scene/SceneFramebuffers.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_CUBEMAPS)
#include "Scene/SceneCubemaps.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_PARTICLES)
#include "Scene/SceneParticles.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_OMNI_SHADOWS)
#include "Scene/SceneOmniShadows.h"
#include "Renderer/RendererOmniShadows.h"
#elif defined(SCENE_VOXEL_TERRAIN)
#include "Scene/SceneVoxelTerrain.h"
#include "Renderer/RendererVoxelTerrain.h"
#elif defined(SCENE_PROCEDURAL_LANDMASS)
#include "Scene/SceneProceduralLandmass.h"
#include "Renderer/RendererVoxelTerrain.h"
#elif defined(SCENE_VOXEL_TERRAIN_SL)
#include "Scene/SceneVoxelTerrainSL.h"
#include "Renderer/RendererVoxelTerrain.h"
#elif defined(SCENE_MARCHING_CUBES)
#include "Scene/SceneMarchingCubes.h"
#include "Renderer/RendererVoxelTerrain.h"
#elif defined(SCENE_SSAO)
#include "Scene/SceneSSAO.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_BLOOM)
#include "Scene/SceneBloom.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_DEFERRED)
#include "Scene/SceneDeferred.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_DEFERRED_OGL)
#include "Scene/SceneDeferredOGL.h"
#include "Renderer/RendererDeferredOGL.h"
#elif defined(SCENE_EDITOR)
#include "Scene/SceneEditor.h"
#include "Renderer/RendererEditor.h"
#elif defined(SCENE_EDITOR_IMGUIZMO)
#include "Scene/SceneEditorImGuizmo.h"
#include "Renderer/RendererEditor.h"
#elif defined(SCENE_ANIM_PBR)
#include "Scene/SceneAnimPBR.h"
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_HAZEL_ENV_MAP)
#include "Scene/SceneHazelEnvMap.h"
#include "Renderer/RendererECS.h"
#elif defined(SCENE_HAZEL_VULKAN)
#include "Scene/SceneHazelVulkan.h"
#include "Renderer/RendererHazelVulkan.h"
#elif defined(SCENE_DX11)
#include "Scene/SceneDX11.h"
#include "Renderer/RendererTrivial.h"
#endif
