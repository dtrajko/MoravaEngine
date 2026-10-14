#pragma once

// The engine is compiled for one scene, which runs on one graphics API, and for the graphics APIs switched on in CMake
// (MoravaEngine/CMakeLists.txt): the MORAVA_SCENE setting defines SCENE_<name> below; GRAPHICS_API_VULKAN,
// GRAPHICS_API_OPENGL and GRAPHICS_API_DIRECTX11 are defined for the scene's API and for every API switched on with
// MORAVA_VULKAN, MORAVA_OPENGL or MORAVA_DIRECTX11 (any combination). For example:
//   cmake -DMORAVA_SCENE=EDITOR build          the EDITOR scene (OpenGL): only the OpenGL classes
//   cmake -DMORAVA_OPENGL=ON build             the OpenGL classes in addition to the scene's
// Only the classes of those APIs are compiled: code that names a class of one API stands inside
// #if defined(GRAPHICS_API_...). Which API runs is decided at startup (main.cpp, RendererAPI_H2M::Current()).

// BEGIN compile the engine for the chosen graphics APIs only

#if !defined(GRAPHICS_API_VULKAN) && !defined(GRAPHICS_API_OPENGL) && !defined(GRAPHICS_API_DIRECTX11)
	#error No graphics API: build with CMake, which defines GRAPHICS_API_... from its MORAVA_SCENE setting
#endif

enum class GraphicsAPI
{
	None,
	Vulkan,
	OpenGL,
	DirectX11,
	DirectX12,
	Metal,
	WebGPU
};

// The graphics API of the scene
#if defined(SCENE_ENV_MAP_VULKAN) || defined(SCENE_HAZEL_VULKAN)
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::Vulkan;
#elif defined(SCENE_DX11)
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::DirectX11;
#else
	constexpr GraphicsAPI CurrentGraphicsAPI = GraphicsAPI::OpenGL;
#endif

// END compile the engine for the chosen graphics APIs only


// The scenes (CMake's MORAVA_SCENE setting defines one of them; ENV_MAP_VULKAN unless set otherwise):
//   OpenGL:      SCENE_COTTAGE, SCENE_EIFFEL, SCENE_SPONZA, SCENE_TERRAIN, SCENE_PBR, SCENE_LEARN_OPENGL, SCENE_BULLET,
//                SCENE_INSTANCED, SCENE_ASTEROIDS, SCENE_NANOSUIT, SCENE_FRAMEBUFFERS, SCENE_CUBEMAPS, SCENE_PARTICLES,
//                SCENE_OMNI_SHADOWS, SCENE_VOXEL_TERRAIN, SCENE_PROCEDURAL_LANDMASS, SCENE_VOXEL_TERRAIN_SL,
//                SCENE_MARCHING_CUBES, SCENE_SSAO, SCENE_BLOOM, SCENE_DEFERRED, SCENE_DEFERRED_OGL, SCENE_ANIM_PBR,
//                SCENE_EDITOR, SCENE_EDITOR_IMGUIZMO, SCENE_HAZEL_ENV_MAP
//   DirectX 11:  SCENE_DX11 (Windows only)
//   Vulkan:      SCENE_HAZEL_VULKAN, SCENE_ENV_MAP_VULKAN


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
	ENV_MAP_VULKAN
};


class Scene;
class RendererBasic;

struct SceneProperties
{
	Scene* Scene;
	RendererBasic* Renderer;
	SceneName Name;
	// H2M::RendererAPITypeH2M RendererAPI;
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
#define OPENGL_4_5_PLUS // allows the use of 'ultramodern' OpenGL 4.5+ functions e.g. glCreateTextures
#elif defined(SCENE_DX11)
#if defined(GRAPHICS_API_DIRECTX11)
#include "Scene/SceneDX11.h"
#endif
#include "Renderer/RendererTrivial.h"
#define OPENGL_4_5_PLUS // allows the use of 'ultramodern' OpenGL 4.5+ functions e.g. glCreateTextures
#elif defined(SCENE_HAZEL_VULKAN)
#if defined(GRAPHICS_API_VULKAN)
#include "Scene/SceneHazelVulkan.h"
#endif
#include "Renderer/RendererTrivial.h"
#elif defined(SCENE_ENV_MAP_VULKAN)
#if defined(GRAPHICS_API_VULKAN)
#include "Scene/SceneEnvMapVulkan.h"
#endif
#include "Renderer/RendererECS.h"
#endif
