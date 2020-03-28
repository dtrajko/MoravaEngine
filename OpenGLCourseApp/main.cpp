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
#include "SceneCottage.h"
#include "SceneEiffel.h"
#include "SceneSponza.h"
#include "SceneTerrain.h"
#include "ScenePBR.h"
#include "SceneJoey.h"
#include "SceneBullet.h"
#include "LightManager.h"
#include "WaterManager.h"
#include "Renderer.h"
#include "RendererPBR.h"
#include "RendererJoey.h"


// Window dimensions
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
};

SceneName currentScene = SceneName::Bullet;

GLfloat deltaTime = 0.0f;
GLfloat lastTime = 0.0f;


int main()
{
	mainWindow = Window(WIDTH, HEIGHT);
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
	}

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		(float)mainWindow.GetBufferWidth() / (float)mainWindow.GetBufferHeight(),
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferWidth());

	renderer->Init();

	ImGuiWrapper::Init(&mainWindow);

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		GLfloat now = (GLfloat)glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		scene->GetCamera()->KeyControl(mainWindow.getKeys(), deltaTime);
		scene->GetCamera()->MouseControl(mainWindow.getMouseButtons(), mainWindow.getXChange(), mainWindow.getYChange());
		scene->GetCamera()->MouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		if (mainWindow.getKeys()[GLFW_KEY_F])
		{
			LightManager::spotLights[2].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		ImGuiWrapper::Begin();

		scene->Update(now, mainWindow);

		renderer->Render(deltaTime, mainWindow, scene, projectionMatrix);

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
