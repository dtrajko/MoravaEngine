#include "SceneFramebuffers.h"

#include "ImGuiWrapper.h"


SceneFramebuffers::SceneFramebuffers()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 10.5f, 10.0f);
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
}

void SceneFramebuffers::SetSkybox()
{
}

void SceneFramebuffers::SetTextures()
{
}

void SceneFramebuffers::SetupMeshes()
{
}

void SceneFramebuffers::SetupModels()
{
}

void SceneFramebuffers::Update(float timestep, Window& mainWindow)
{
}

void SceneFramebuffers::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
}

void SceneFramebuffers::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneFramebuffers::~SceneFramebuffers()
{
}
