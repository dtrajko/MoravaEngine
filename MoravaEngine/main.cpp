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

// #define SPIR_ENABLED

// #define SCENE_COTTAGE
// #define SCENE_EIFFEL
// #define SCENE_SPONZA
// #define SCENE_TERRAIN
// #define SCENE_PBR
// #define SCENE_LEARN_OPEN_GL
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
#define SCENE_SSAO
// #define SCENE_DEFERRED
// #define SCENE_EDITOR
// #define SCENE_EDITOR_IMGUIZMO
// #define SCENE_ANIM_PBR
// #define SCENE_DEFERRED_OGL
// #define SCENE_HAZEL_ENV_MAP

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
#elif defined(SCENE_LEARN_OPEN_GL)
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
#endif

#include <GL/glew.h>

#include <memory>


// Window dimensions
const char* windowTitle = "3D Graphics Engine (C++ / OpenGL)";
const int WIDTH = 1280;
const int HEIGHT = 720;

Scene* scene;
RendererBasic* renderer;

// Key cooldown time (emulate onKeyReleased)
EventCooldown keyPressCooldown = { 0.0f, 0.2f };


int main()
{
	Log::Init();

	Application::Get()->InitWindow(WindowProps({ windowTitle, WIDTH, HEIGHT }));

#if defined(SPIR_ENABLED)
	RendererBasic::SetSpirVEnabled(true);
#else
	RendererBasic::SetSpirVEnabled(false);
#endif

	LOG_INFO("OpenGL Info:");
	LOG_INFO("   Vendor: {0}",   glGetString(GL_VENDOR));
	LOG_INFO("   Renderer: {0}", glGetString(GL_RENDERER));
	LOG_INFO("   Version: {0}",  glGetString(GL_VERSION));

#if defined(SCENE_COTTAGE)
	scene = new SceneCottage();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_EIFFEL)
	scene = new SceneEiffel();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_SPONZA)
	scene = new SceneSponza();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_TERRAIN)
	scene = new SceneTerrain();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_PBR)
	scene = new ScenePBR();
	renderer = static_cast<RendererBasic*>(new RendererPBR());
#elif defined(SCENE_LEARN_OPEN_GL)
	scene = new SceneJoey();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_BULLET)
	scene = new SceneBullet();
	renderer = static_cast<RendererBasic*>(new Renderer());
#elif defined(SCENE_INSTANCED)
	scene = new SceneInstanced();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_ASTEROIDS)
	scene = new SceneAsteroids();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_NANOSUIT)
	scene = new SceneNanosuit();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_FRAMEBUFFERS)
	scene = new SceneFramebuffers();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_CUBEMAPS)
	scene = new SceneCubemaps();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_PARTICLES)
	scene = new SceneParticles();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_OMNI_SHADOWS)
	scene = new SceneOmniShadows();
	renderer = static_cast<RendererBasic*>(new RendererOmniShadows());
#elif defined(SCENE_PROCEDURAL_LANDMASS)
	scene = new SceneProceduralLandmass();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_VOXEL_TERRAIN)
	scene = new SceneVoxelTerrain();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_VOXEL_TERRAIN_SL)
	scene = new SceneVoxelTerrainSL();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_MARCHING_CUBES)
	scene = new SceneMarchingCubes();
	renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
#elif defined(SCENE_SSAO)
	scene = new SceneSSAO();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_ANIM_PBR)
	scene = new SceneAnimPBR();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_DEFERRED)
	scene = new SceneDeferred();
	renderer = static_cast<RendererBasic*>(new RendererTrivial());
#elif defined(SCENE_DEFERRED_OGL)
	scene = new SceneDeferredOGL();
	renderer = static_cast<RendererBasic*>(new RendererDeferredOGL());
#elif defined(SCENE_EDITOR)
	scene = new SceneEditor();
	renderer = static_cast<RendererBasic*>(new RendererEditor());
#elif defined(SCENE_EDITOR_IMGUIZMO)
	scene = new SceneEditorImGuizmo();
	renderer = static_cast<RendererBasic*>(new RendererEditor());
#elif defined(SCENE_HAZEL_ENV_MAP)
	scene = new SceneHazelEnvMap();
	renderer = static_cast<RendererBasic*>(new RendererECS());
#else
		throw std::runtime_error("Scene and Renderer could not be loaded!");
#endif

	Application::Get()->SetScene(scene);
	Application::Get()->SetRenderer(renderer);

	// experimental, testing Hazel event system
	Application::Run();

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)Application::Get()->GetWindow()->GetWidth() / (float)Application::Get()->GetWindow()->GetHeight(),
		Scene::GetSettings().nearPlane, Scene::GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)Application::Get()->GetWindow()->GetWidth(), (int)Application::Get()->GetWindow()->GetHeight());

	renderer->Init(scene);

	ImGuiWrapper::Init(Application::Get()->GetWindow());

	float targetFPS = 60.0f;
	float targetUpdateRate = 24.0f;
	Timer timer(targetFPS, targetUpdateRate);

	// Loop until window closed
	while (!Application::Get()->GetWindow()->GetShouldClose())
	{
		Timer::Get()->Update();

		scene->GetCameraController()->Update();

		MousePicker::Get()->Update(scene->GetCamera()->GetViewMatrix());

		if (ImGuiWrapper::CanViewportReceiveEvents())
		{
			// Toggle wireframe mode
			if (Input::IsKeyPressed(Key::R))
			{
				if (Timer::Get()->GetCurrentTimestamp() - keyPressCooldown.lastTime > keyPressCooldown.cooldown)
				{
					scene->SetWireframeEnabled(!scene->IsWireframeEnabled());
					keyPressCooldown.lastTime = Timer::Get()->GetCurrentTimestamp();
				}
			}

			// Flashlight toggle key
			if (Input::IsKeyPressed(Key::F))
			{
				LightManager::spotLights[2].GetBasePL()->Toggle();
				// Application::Get()->GetWindow()->getKeys()[GLFW_KEY_L] = false;
			}
		}

		if (scene->IsWireframeEnabled()) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		ImGuiWrapper::Begin();

		{
			Profiler profiler("Scene::Update");
			// if (Timer::Get()->CanUpdate())
			scene->Update(Timer::Get()->GetCurrentTimestamp(), Application::Get()->GetWindow()); // TODO deltaTime obsolete
			scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		{
			Profiler profiler("Renderer::Render");
			// if (Timer::Get()->CanRender())
			renderer->Render(Timer::Get()->GetDeltaTime(), Application::Get()->GetWindow(), scene, projectionMatrix); // TODO deltaTime obsolete
			scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), Application::Get()->GetWindow());

		scene->GetProfilerResults()->clear();

		ImGuiWrapper::End();

		glDisable(GL_BLEND);

		// Swap buffers and poll events
		Application::Get()->GetWindow()->OnUpdate();
	}

	ImGuiWrapper::Cleanup();

	// delete scene;
	delete renderer;

	return 0;
}
