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

#include "CommonValues.h"
#include "Application.h"
#include "ImGuiWrapper.h"
#include "Window.h"
#include "CameraController.h"
#include "LightManager.h"
#include "WaterManager.h"
#include "Profiler.h"
#include "MousePicker.h"
#include "Timer.h"
#include "Log.h"

#include "SceneCottage.h"
#include "SceneEiffel.h"
#include "SceneSponza.h"
#include "SceneTerrain.h"
#include "ScenePBR.h"
#include "SceneJoey.h"
#include "SceneBullet.h"
#include "SceneInstanced.h"
#include "SceneAsteroids.h"
#include "SceneNanosuit.h"
#include "SceneFramebuffers.h"
#include "SceneCubemaps.h"
#include "SceneParticles.h"
#include "SceneOmniShadows.h"
#include "SceneEditor.h"
#include "SceneProceduralLandmass.h"
#include "SceneVoxelTerrain.h"
#include "SceneVoxelTerrainSL.h"
#include "SceneMarchingCubes.h"
#include "SceneEditorFramebuffer.h"
#include "SceneSSAO.h"
#include "SceneAnimPBR.h"

#include "Renderer.h"
#include "RendererPBR.h"
#include "RendererJoey.h"
#include "RendererInstanced.h"
#include "RendererAsteroids.h"
#include "RendererNanosuit.h"
#include "RendererFramebuffers.h"
#include "RendererCubemaps.h"
#include "RendererOmniShadows.h"
#include "RendererVoxelTerrain.h"
#include "RendererEditor.h"
#include "RendererEditorFramebuffer.h"
#include "RendererSSAO.h"
#include "RendererTrivial.h"


// Window dimensions
const char* windowTitle = "3D Graphics Engine (C++ / OpenGL)";
const int WIDTH = 1280;
const int HEIGHT = 720;


Window mainWindow;
Scene* scene;
RendererBasic* renderer;


enum class SceneName
{
	Cottage,
	Eiffel,
	Sponza,
	Terrain,
	PBR,
	LearnOpenGL,
	Bullet,
	Instanced,
	Asteroids,
	Nanosuit,
	Framebuffers,
	Cubemaps,
	Particles,
	OmniShadows,
	Editor,
	VoxelTerrain,
	ProceduralLandmass,
	VoxelTerrainSL,
	MarchingCubes,
	EditorFramebuffer,
	SSAO,
	AnimPBR,
};

SceneName currentScene = SceneName::LearnOpenGL;

// Key cooldown time (emulate onKeyReleased)
EventCooldown keyPressCooldown = { 0.0f, 0.2f };


int main()
{
	Log::Init();

	mainWindow = Window(WIDTH, HEIGHT, windowTitle);
	mainWindow.Initialize();

	Application::Get()->SetWindow(&mainWindow);

	LOG_INFO("OpenGL Info:");
	LOG_INFO("   Vendor: {0}",   glGetString(GL_VENDOR));
	LOG_INFO("   Renderer: {0}", glGetString(GL_RENDERER));
	LOG_INFO("   Version: {0}",  glGetString(GL_VERSION));

	switch (currentScene)
	{
	case SceneName::Cottage:
		scene = new SceneCottage();
		renderer = static_cast<RendererBasic*>(new Renderer());
		break;
	case SceneName::Eiffel:
		scene = new SceneEiffel();
		renderer = static_cast<RendererBasic*>(new Renderer());
		break;
	case SceneName::Sponza:
		scene = new SceneSponza();
		renderer = static_cast<RendererBasic*>(new Renderer());
		break;
	case SceneName::Terrain:
		scene = new SceneTerrain();
		renderer = static_cast<RendererBasic*>(new Renderer());
		break;
	case SceneName::PBR:
		scene = new ScenePBR();
		renderer = static_cast<RendererBasic*>(new RendererPBR());
		break;
	case SceneName::LearnOpenGL:
		scene = new SceneJoey();
		renderer = static_cast<RendererBasic*>(new RendererJoey());
		break;
	case SceneName::Bullet:
		scene = new SceneBullet();
		renderer = static_cast<RendererBasic*>(new Renderer());
		break;
	case SceneName::Instanced:
		scene = new SceneInstanced();
		renderer = static_cast<RendererBasic*>(new RendererInstanced());
		break;
	case SceneName::Asteroids:
		scene = new SceneAsteroids();
		renderer = static_cast<RendererBasic*>(new RendererAsteroids());
		break;
	case SceneName::Nanosuit:
		scene = new SceneNanosuit();
		renderer = static_cast<RendererBasic*>(new RendererNanosuit());
		break;
	case SceneName::Framebuffers:
		scene = new SceneFramebuffers();
		renderer = static_cast<RendererBasic*>(new RendererFramebuffers());
		break;
	case SceneName::Cubemaps:
		scene = new SceneCubemaps();
		renderer = static_cast<RendererBasic*>(new RendererCubemaps());
		break;
	case SceneName::Particles:
		scene = new SceneParticles();
		renderer = static_cast<RendererBasic*>(new RendererEditor());
		break;
	case SceneName::OmniShadows:
		scene = new SceneOmniShadows();
		renderer = static_cast<RendererBasic*>(new RendererOmniShadows());
		break;
	case SceneName::Editor:
		scene = new SceneEditor();
		renderer = static_cast<RendererBasic*>(new RendererEditor());
		break;
	case SceneName::ProceduralLandmass:
		scene = new SceneProceduralLandmass();
		renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
		break;
	case SceneName::VoxelTerrain:
		scene = new SceneVoxelTerrain();
		renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
		break;
	case SceneName::VoxelTerrainSL:
		scene = new SceneVoxelTerrainSL();
		renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
		break;
	case SceneName::MarchingCubes:
		scene = new SceneMarchingCubes();
		renderer = static_cast<RendererBasic*>(new RendererVoxelTerrain());
		break;
	case SceneName::EditorFramebuffer:
		scene = new SceneEditorFramebuffer();
		renderer = static_cast<RendererBasic*>(new RendererEditorFramebuffer());
		break;
	case SceneName::SSAO:
		scene = new SceneSSAO();
		renderer = static_cast<RendererBasic*>(new RendererSSAO());
		break;
	case SceneName::AnimPBR:
		scene = new SceneAnimPBR();
		renderer = static_cast<RendererBasic*>(new RendererTrivial());
		break;
	default:
		throw std::runtime_error("Scene and Renderer could not be loaded!");
	}

	Application::Get()->SetScene(scene);
	Application::Get()->SetRenderer(renderer);

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
		Scene::GetSettings().nearPlane, Scene::GetSettings().farPlane);

	RendererBasic::SetProjectionMatrix(projectionMatrix);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferHeight());

	renderer->Init(scene);

	ImGuiWrapper::Init(&mainWindow);

	float targetFPS = 60.0f;
	float targetUpdateRate = 24.0f;
	Timer timer(targetFPS, targetUpdateRate);

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		Timer::Get()->Update();

		scene->GetCameraController()->KeyControl(mainWindow.getKeys(), Timer::Get()->GetDeltaTime());
		scene->GetCameraController()->MouseControl(mainWindow.getMouseButtons(), mainWindow.getXChange(), mainWindow.getYChange());
		scene->GetCameraController()->MouseScrollControl(mainWindow.getKeys(), Timer::Get()->GetDeltaTime(), mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		MousePicker::Get()->Update(mainWindow.GetMouseX(), mainWindow.GetMouseY(),
			(float)mainWindow.GetBufferWidth(), (float)mainWindow.GetBufferHeight(),
			RendererBasic::GetProjectionMatrix(), scene->GetCameraController()->CalculateViewMatrix());

		if (mainWindow.getKeys()[GLFW_KEY_F])
		{
			LightManager::spotLights[2].GetBasePL()->Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		// Toggle wireframe mode
		if (mainWindow.getKeys()[GLFW_KEY_R] && !mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL])
		{
			if (Timer::Get()->GetCurrentTimestamp() - keyPressCooldown.lastTime > keyPressCooldown.cooldown)
			{
				scene->SetWireframeEnabled(!scene->IsWireframeEnabled());
				keyPressCooldown.lastTime = Timer::Get()->GetCurrentTimestamp();
			}
		}

		if (scene->IsWireframeEnabled())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ImGuiWrapper::Begin();

		{
			Profiler profiler("Scene::Update");
			// if (Timer::Get()->CanUpdate())
			scene->Update(Timer::Get()->GetCurrentTimestamp(), mainWindow); // TODO deltaTime obsolete
			scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		{
			Profiler profiler("Renderer::Render");
			// if (Timer::Get()->CanRender())
			renderer->Render(Timer::Get()->GetDeltaTime(), mainWindow, scene, projectionMatrix); // TODO deltaTime obsolete
			scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		scene->UpdateImGui(Timer::Get()->GetCurrentTimestamp(), mainWindow);

		scene->GetProfilerResults()->clear();

		ImGuiWrapper::End();

		glDisable(GL_BLEND);

		// Swap buffers
		mainWindow.SwapBuffers();

		// Get and handle user input events
		glfwPollEvents();
	}

	ImGuiWrapper::Cleanup();

	// delete scene;
	delete renderer;

	return 0;
}
