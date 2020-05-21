#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"
#include "ImGuiWrapper.h"
#include "Window.h"
#include "Camera.h"
#include "LightManager.h"
#include "WaterManager.h"
#include "Profiler.h"
#include "MousePicker.h"

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
#include "SceneEditor.h"

#include "Renderer.h"
#include "RendererPBR.h"
#include "RendererJoey.h"
#include "RendererInstanced.h"
#include "RendererAsteroids.h"
#include "RendererNanosuit.h"
#include "RendererFramebuffers.h"
#include "RendererCubemaps.h"
#include "RendererEditor.h"


// Window dimensions
const char* windowTitle = "3D Graphics Engine (C++ / OpenGL)";
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

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
	Editor,
};

SceneName currentScene = SceneName::Editor;

float deltaTime = 0.0f;
float lastTimestamp = 0.0f;
float lastUpdateTime = 0.0f;
float updateInterval = 0.0416f; // 24 times per second
bool shouldUpdate = false;

// Key cooldown time (emulate onKeyReleased)
float m_KeyCooldownTime = 0.2f;
float m_LastKeyPressTime = 0.0f;

// Profiler results
std::map<const char*, float> profilerResults;


int main()
{
	mainWindow = Window(WIDTH, HEIGHT, windowTitle);
	mainWindow.Initialize();

	printf("OpenGL Info:\n");
	printf("   Vendor: %s\n", glGetString(GL_VENDOR));
	printf("   Renderer: %s\n", glGetString(GL_RENDERER));
	printf("   Version: %s\n", glGetString(GL_VERSION));

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
	case SceneName::Editor:
		scene = new SceneEditor();
		renderer = static_cast<RendererBasic*>(new RendererEditor());
		break;
	default:
		throw std::runtime_error("Scene and Renderer could not be loaded!");
	}

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferHeight());

	renderer->Init(scene);

	ImGuiWrapper::Init(&mainWindow);

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		float currentTimestamp = (float)glfwGetTime();
		deltaTime = currentTimestamp - lastTimestamp;
		lastTimestamp = currentTimestamp;

		shouldUpdate = false;
		if (currentTimestamp - lastUpdateTime > updateInterval)
		{
			shouldUpdate = true;
			lastUpdateTime = currentTimestamp;
		}

		scene->GetCamera()->KeyControl(mainWindow.getKeys(), deltaTime);
		scene->GetCamera()->MouseControl(mainWindow.getMouseButtons(), mainWindow.getXChangeReset(), mainWindow.getYChangeReset());
		scene->GetCamera()->MouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		MousePicker::Get()->Update(mainWindow.GetMouseX(), mainWindow.GetMouseY(),
			(float)mainWindow.GetBufferWidth(), (float)mainWindow.GetBufferHeight(), projectionMatrix, scene->GetCamera()->CalculateViewMatrix());

		if (mainWindow.getKeys()[GLFW_KEY_F])
		{
			LightManager::spotLights[2].GetBasePL()->Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		// Toggle wireframe mode
		if (mainWindow.getKeys()[GLFW_KEY_R] && !mainWindow.getKeys()[GLFW_KEY_LEFT_CONTROL])
		{
			if (currentTimestamp - m_LastKeyPressTime > m_KeyCooldownTime)
			{
				scene->SetWireframeEnabled(!scene->IsWireframeEnabled());
				m_LastKeyPressTime = currentTimestamp;
			}
		}

		if (scene->IsWireframeEnabled())
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ImGuiWrapper::Begin();

		{
			Profiler profiler("Scene::Update");
			scene->Update(currentTimestamp, mainWindow);
			profilerResults.insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		{
			Profiler profiler("Renderer::Render");
			renderer->Render(deltaTime, mainWindow, scene, projectionMatrix);
			profilerResults.insert(std::make_pair(profiler.GetName(), profiler.Stop()));
		}

		scene->UpdateImGui(currentTimestamp, mainWindow, profilerResults);

		profilerResults.clear();

		ImGuiWrapper::End();

		glDisable(GL_BLEND);

		// Swap buffers
		mainWindow.SwapBuffers();

		// Get and handle user input events
		glfwPollEvents();
	}

	ImGuiWrapper::Cleanup();

	delete scene;
	delete renderer;

	return 0;
}
