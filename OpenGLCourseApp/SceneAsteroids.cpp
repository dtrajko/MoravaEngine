#include "SceneAsteroids.h"

#include "ImGuiWrapper.h"

#include "RendererAsteroids.h"


SceneAsteroids::SceneAsteroids()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 16.0f, 28.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
}

void SceneAsteroids::SetSkybox()
{
}

void SceneAsteroids::SetTextures()
{
}

void SceneAsteroids::SetupMeshes()
{
}

void SceneAsteroids::SetupModels()
{ 
}

void SceneAsteroids::Update(float timestep, Window& mainWindow)
{
}

void SceneAsteroids::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
}

void SceneAsteroids::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneAsteroids::~SceneAsteroids()
{
}
