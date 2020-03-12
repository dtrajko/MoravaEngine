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
Camera* camera;

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

	camera = new Camera(scene->GetSettings().cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f), scene->GetSettings().cameraStartYaw, 0.0f, scene->GetSettings().cameraMoveSpeed, 0.1f);

	// Projection matrix
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f),
		mainWindow.GetBufferWidth() / mainWindow.GetBufferHeight(), 
		scene->GetSettings().nearPlane, scene->GetSettings().farPlane);

	LightManager* lightManager = new LightManager(scene->GetSettings());

	// Water framebuffers
	WaterManager* waterManager = new WaterManager((int)mainWindow.GetBufferWidth(), (int)mainWindow.GetBufferHeight(),
		scene->GetSettings().waterHeight, scene->GetSettings().waterWaveSpeed);

	Renderer::Init();	

	ImGuiWrapper::Init(&mainWindow);

	// Loop until window closed
	while (!mainWindow.GetShouldClose())
	{
		GLfloat now = (GLfloat)glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		camera->KeyControl(mainWindow.getKeys(), deltaTime);
		camera->MouseControl(mainWindow.getMouseButtons(), mainWindow.getXChange(), mainWindow.getYChange());
		// camera->mouseScrollControl(mainWindow.getKeys(), deltaTime, mainWindow.getXMouseScrollOffset(), mainWindow.getYMouseScrollOffset());

		if (mainWindow.getKeys()[GLFW_KEY_L])
		{
			LightManager::spotLights[2].Toggle();
			mainWindow.getKeys()[GLFW_KEY_L] = false;
		}

		ImGuiWrapper::Begin();

		scene->Update(now, camera, *lightManager, waterManager);

		Renderer::RenderPassShadow(&LightManager::directionalLight, camera->CalculateViewMatrix(), projectionMatrix, scene, waterManager);
		Renderer::RenderOmniShadows(camera->CalculateViewMatrix(), projectionMatrix, scene, waterManager);
		Renderer::RenderWaterEffects(waterManager, projectionMatrix, scene, camera, deltaTime);
		Renderer::RenderPass(projectionMatrix, mainWindow, scene, camera, waterManager);

		ImGuiWrapper::End();

		Renderer::GetShaders()["main"]->Unbind();

		glDisable(GL_BLEND);

		// Swap buffers
		mainWindow.SwapBuffers();

		// Get and handle user input events
		glfwPollEvents();
	}

	ImGuiWrapper::Cleanup();

	delete lightManager;
	delete scene;
	delete camera;

	return 0;
}
