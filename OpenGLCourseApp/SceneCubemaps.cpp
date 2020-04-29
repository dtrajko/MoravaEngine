#include "SceneCubemaps.h"

#include "ImGuiWrapper.h"


SceneCubemaps::SceneCubemaps()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 1.0f, 5.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	SetCamera();
	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
	SetGeometry();
}

void SceneCubemaps::SetSkybox()
{
}

void SceneCubemaps::SetTextures()
{
}

void SceneCubemaps::SetupMeshes()
{
}

void SceneCubemaps::SetupModels()
{
}

void SceneCubemaps::SetGeometry()
{
}

void SceneCubemaps::Update(float timestep, Window& mainWindow)
{
}

void SceneCubemaps::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
}

void SceneCubemaps::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

void SceneCubemaps::CleanupGeometry()
{
}

SceneCubemaps::~SceneCubemaps()
{
	CleanupGeometry();
}
