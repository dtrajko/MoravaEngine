#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "CommonValues.h"
#include "Window.h"
#include "Camera.h"
#include "SceneCottage.h"
#include "SceneEiffel.h"
#include "SceneSponza.h"
#include "LightManager.h"
#include "Renderer.h"
#include "WaterManager.h"



// Window dimensions
const GLint WIDTH = 1280;
const GLint HEIGHT = 720;
const float toRadians = 3.14159265f / 180.0f;

Window mainWindow;
Scene* scene;
Camera* camera;

std::string currentScene = "eiffel"; // "cottage", "eiffel", "sponza"

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

	camera = new Camera(scene->GetSettings().cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), scene->GetSettings().cameraStartYaw, 0.0f, scene->GetSettings().cameraMoveSpeed, 0.1f);

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 0.1f, 200.0f);

	LightManager* lightManager = new LightManager(scene->GetSettings());

	// Water framebuffers
	WaterManager* waterManager = new WaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferHeight(),
		scene->GetSettings().waterHeight, scene->GetSettings().waterWaveSpeed);

	Renderer::Init();

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		GLfloat now = (GLfloat)glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		// Get and handle user input events
		glfwPollEvents();

		camera->keyControl(mainWindow.getKeys(), deltaTime);
		camera->mouseControl(mainWindow.getMouseButtons(), mainWindow.getXChange(), mainWindow.getYChange());
		// camera->mouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		if (mainWindow.getKeys()[GLFW_KEY_L])
		{
			LightManager::spotLights[2].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		scene->Update(now, *lightManager);

		Renderer::RenderPassShadow(&LightManager::directionalLight, camera->CalculateViewMatrix(), projectionMatrix, scene, waterManager);
		Renderer::RenderOmniShadows(camera->CalculateViewMatrix(), projectionMatrix, scene, waterManager);
		Renderer::RenderWaterEffects(waterManager, projectionMatrix, scene, camera, deltaTime);
		Renderer::RenderPass(projectionMatrix, mainWindow, scene, camera, waterManager);

		Renderer::GetShaders()["main"]->Unbind();
		mainWindow.SwapBuffers();
	}

	delete lightManager;
	delete scene;
	delete camera;

	return 0;
}
