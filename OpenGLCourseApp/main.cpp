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
#include "Renderer.h"
#include "LightManager.h"
#include "WaterManager.h"


// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
Scene* scene;
Renderer* renderer;

std::string currentScene = "pbr"; // "cottage", "eiffel", "sponza", "terrain", "pbr"

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

	if (currentScene == "cottage")
		scene = new SceneCottage();
	else if (currentScene == "eiffel")
		scene = new SceneEiffel();
	else if (currentScene == "sponza")
		scene = new SceneSponza();
	else if (currentScene == "terrain")
		scene = new SceneTerrain();
	else if (currentScene == "pbr")
		scene = new ScenePBR();

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	scene->SetCamera();
	scene->SetLightManager();
	scene->SetWaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferWidth());

	renderer = new Renderer();
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
		// scene->GetCamera()->MouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		if (mainWindow.getKeys()[GLFW_KEY_L])
		{
			LightManager::spotLights[2].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		ImGuiWrapper::Begin();

		scene->Update(now);

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
