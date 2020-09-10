#include "SceneInstanced.h"

#include "ImGuiWrapper.h"

#include "RendererInstanced.h"


SceneInstanced::SceneInstanced()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 2.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneInstanced::SetSkybox()
{
}

void SceneInstanced::SetupTextures()
{
}

void SceneInstanced::SetupMeshes()
{
}

void SceneInstanced::SetupModels()
{ 
}

void SceneInstanced::Update(float timestep, Window& mainWindow)
{
}

void SceneInstanced::UpdateImGui(float timestep, Window& mainWindow)
{
}

void SceneInstanced::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
}

SceneInstanced::~SceneInstanced()
{
}
