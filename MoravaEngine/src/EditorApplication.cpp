#include "EditorApplication.h"

#include "H2M/ImGui/ImGuiLayerH2M.h"
#include "H2M/Project/UserPreferencesH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLRendererH2M.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanRendererH2M.h"
#include "H2M/Platform/Vulkan/VulkanTestLayer.h"

#include "EnvMapVulkan/EnvMapVulkanEditorLayer.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"
#endif
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11Renderer.h"
#include "Platform/DX11/DX11TestLayer.h"
#endif

// The scene chosen in config.h (or by CMake's MORAVA_SCENE), and its renderer
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
#elif defined(SCENE_DX11)
#if defined(GRAPHICS_API_DIRECTX11)
#include "Scene/SceneDX11.h"
#endif
#include "Renderer/RendererTrivial.h"
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


SceneProperties EditorApplication::CreateScene()
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
#elif defined(SCENE_DX11)
	sceneProperties.Scene = new SceneDX11();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::HAZEL_VULKAN;
#elif defined(SCENE_HAZEL_VULKAN)
	sceneProperties.Scene = new SceneHazelVulkan();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererTrivial());
	sceneProperties.Name = SceneName::HAZEL_VULKAN;
#elif defined(SCENE_ENV_MAP_VULKAN)
	sceneProperties.Scene = new SceneEnvMapVulkan();
	sceneProperties.Renderer = static_cast<RendererBasic*>(new RendererECS());
	sceneProperties.Name = SceneName::ENV_MAP_VULKAN;
#else
	sceneProperties.Scene = nullptr;
	sceneProperties.Renderer = nullptr;
	sceneProperties.Name = SceneName::NONE;
	throw std::runtime_error("Scene and Renderer could not be loaded!");
#endif

	return sceneProperties;
}

H2M::RendererAPI_H2M* EditorApplication::CreateRendererAPI()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
#if defined(GRAPHICS_API_OPENGL)
		case H2M::RendererAPITypeH2M::OpenGL: return new H2M::OpenGLRendererH2M();
#endif
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan:
			switch (Application::Get()->GetSceneProperties().Name)
			{
				case SceneName::HAZEL_VULKAN: return new H2M::VulkanRendererH2M();
				case SceneName::ENV_MAP_VULKAN: return new EnvMapVulkanRenderer();
				default: break;
			}
			break;
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11: return new DX11Renderer();
#endif
		default: break;
	}
	return nullptr;
}

H2M::ImGuiLayerH2M* EditorApplication::CreateImGuiLayer()
{
	return H2M::ImGuiLayerH2M::Create();
}

void EditorApplication::PushLayers(Application& application)
{
	switch (H2M::RendererAPI_H2M::Current())
	{
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan:
			switch (application.GetSceneProperties().Name)
			{
				case SceneName::HAZEL_VULKAN:
					application.PushLayer(new H2M::VulkanTestLayer("VulkanTestLayer")); // to be removed
					break;
				case SceneName::ENV_MAP_VULKAN:
					application.PushLayer(new EnvMapVulkanEditorLayer(H2M::RefH2M<H2M::UserPreferencesH2M>::Create())); // the editor (src/Editor)
					break;
				default:
					break;
			}
			break;
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:
			application.PushLayer(new DX11TestLayer("DX11TestLayer"));
			break;
#endif
		default: break;
	}
}

void EditorApplication::Draw()
{
	Scene* scene = Application::Get()->GetScene();
	switch (H2M::RendererAPI_H2M::Current())
	{
#if defined(GRAPHICS_API_VULKAN)
		case H2M::RendererAPITypeH2M::Vulkan:
			switch (Application::Get()->GetSceneProperties().Name)
			{
				case SceneName::HAZEL_VULKAN:
					H2M::VulkanRendererH2M::Draw(scene->GetCamera());
					break;
				case SceneName::ENV_MAP_VULKAN:
					EnvMapVulkanRenderer::Draw(scene->GetCamera()); // the editor's camera
					break;
				default: break;
			}
			break;
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case H2M::RendererAPITypeH2M::DX11:
			DX11Renderer::Draw(scene->GetCamera());
			break;
#endif
		default: break;
	}
}

void EditorApplication::OnWindowResize(uint32_t width, uint32_t height)
{
#if defined(GRAPHICS_API_DIRECTX11)
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::DX11)
	{
		DX11Renderer::OnResize(width, height);
	}
#endif
}
