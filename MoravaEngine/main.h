#pragma once

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifndef  NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif // !_WIN32


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

#include "Hazel/Core/Base.h"
#include "Hazel/Events/Event.h"

#include "Camera/CameraController.h"
#include "Core/Application.h"
#include "Core/CommonValues.h"
#include "Core/Input.h"
#include "Core/Log.h"
#include "Core/MousePicker.h"
#include "Core/Profiler.h"
#include "Core/Timer.h"
#include "ImGui/ImGuiWrapper.h"
#include "Light/LightManager.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Core/SceneProperties.h"

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
#endif

#include <GL/glew.h>

#include <memory>


SceneProperties SetSceneProperties()
{
	SceneProperties sceneProperties;

#if defined(SCENE_COTTAGE)
	sceneProperties.Scene = new SceneCottage();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::COTTAGE;
#elif defined(SCENE_EIFFEL)
	sceneProperties.Scene = new SceneEiffel();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::EIFFEL;
#elif defined(SCENE_SPONZA)
	sceneProperties.Scene = new SceneSponza();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::SPONZA;
#elif defined(SCENE_TERRAIN)
	sceneProperties.Scene = new SceneTerrain();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::TERRAIN;
#elif defined(SCENE_PBR)
	sceneProperties.Scene = new ScenePBR();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererPBR());
	sceneProperties.Name = SceneName::PBR;
#elif defined(SCENE_LEARN_OPENGL)
	sceneProperties.Scene = new SceneJoey();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::LEARN_OPENGL;
#elif defined(SCENE_BULLET)
	sceneProperties.Scene = new SceneBullet();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new Renderer());
	sceneProperties.Name = SceneName::BULLET;
#elif defined(SCENE_INSTANCED)
	sceneProperties.Scene = new SceneInstanced();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::INSTANCED;
#elif defined(SCENE_ASTEROIDS)
	sceneProperties.Scene = new SceneAsteroids();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::ASTEROIDS;
#elif defined(SCENE_NANOSUIT)
	sceneProperties.Scene = new SceneNanosuit();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::NANOSUIT;
#elif defined(SCENE_FRAMEBUFFERS)
	sceneProperties.Scene = new SceneFramebuffers();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::FRAMEBUFFERS;
#elif defined(SCENE_CUBEMAPS)
	sceneProperties.Scene = new SceneCubemaps();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::CUBEMAPS;
#elif defined(SCENE_PARTICLES)
	sceneProperties.Scene = new SceneParticles();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::PARTICLES;
#elif defined(SCENE_OMNI_SHADOWS)
	sceneProperties.Scene = new SceneOmniShadows();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererOmniShadows());
	sceneProperties.Name = SceneName::OMNI_SHADOWS;
#elif defined(SCENE_PROCEDURAL_LANDMASS)
	sceneProperties.Scene = new SceneProceduralLandmass();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::PROCEDURAL_LANDMASS;
#elif defined(SCENE_VOXEL_TERRAIN)
	sceneProperties.Scene = new SceneVoxelTerrain();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::VOXEL_TERRAIN;
#elif defined(SCENE_VOXEL_TERRAIN_SL)
	sceneProperties.Scene = new SceneVoxelTerrainSL();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::VOXEL_TERRAIN_SL;
#elif defined(SCENE_MARCHING_CUBES)
	sceneProperties.Scene = new SceneMarchingCubes();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
	sceneProperties.Name = SceneName::MARCHING_CUBES;
#elif defined(SCENE_SSAO)
	sceneProperties.Scene = new SceneSSAO();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::SSAO;
#elif defined(SCENE_BLOOM)
	sceneProperties.Scene = new SceneBloom();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::BLOOM;
#elif defined(SCENE_ANIM_PBR)
	sceneProperties.Scene = new SceneAnimPBR();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::ANIM_PBR;
#elif defined(SCENE_DEFERRED)
	sceneProperties.Scene = new SceneDeferred();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::DEFERRED;
#elif defined(SCENE_DEFERRED_OGL)
	sceneProperties.Scene = new SceneDeferredOGL();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererDeferredOGL());
	sceneProperties.Name = SceneName::DEFERRED_OGL;
#elif defined(SCENE_EDITOR)
	sceneProperties.Scene = new SceneEditor();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererEditor());
	sceneProperties.Name = SceneName::EDITOR;
#elif defined(SCENE_EDITOR_IMGUIZMO)
	sceneProperties.Scene = new SceneEditorImGuizmo();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererEditor());
	sceneProperties.Name = SceneName::EDITOR_IMGUIZMO;
#elif defined(SCENE_HAZEL_ENV_MAP)
	sceneProperties.Scene = new SceneHazelEnvMap();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererECS());
	sceneProperties.Name = SceneName::HAZEL_ENV_MAP;
#elif defined(SCENE_HAZEL_VULKAN)
	sceneProperties.Scene = new SceneHazelVulkan();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererHazelVulkan());
	sceneProperties.Name = SceneName::HAZEL_VULKAN;
#else
	sceneProperties.Scene = nullptr;
	sceneProperties.Renderer = nullptr;
	sceneProperties.Name = SceneName::NONE;
	throw std::runtime_error("Scene and Renderer could not be loaded!");
#endif

	return sceneProperties;
}

// Window properties
const char* WINDOW_TITLE = "Morava Engine";
const int WIDTH = 1280;
const int HEIGHT = 720;
