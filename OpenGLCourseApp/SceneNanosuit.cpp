#include "SceneNanosuit.h"

#include "ImGuiWrapper.h"

#include "RendererNanosuit.h"
#include "LearnOpenGL/ModelJoey.h"


SceneNanosuit::SceneNanosuit()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 10.5f, 10.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
}

void SceneNanosuit::SetSkybox()
{
}

void SceneNanosuit::SetTextures()
{
}

void SceneNanosuit::SetupMeshes()
{
}

void SceneNanosuit::SetupModels()
{
	ModelJoey* nanosuit = new ModelJoey("Models/nanosuit.obj", "Textures/nanosuit");
	models.insert(std::make_pair("nanosuit", nanosuit));
}

void SceneNanosuit::Update(float timestep, Window& mainWindow)
{
}

void SceneNanosuit::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
}

void SceneNanosuit::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneNanosuit::~SceneNanosuit()
{
	for (auto& model : models)
		delete model.second;

	models.clear();
}
