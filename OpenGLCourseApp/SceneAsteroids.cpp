#include "SceneAsteroids.h"

#include "ImGuiWrapper.h"

#include "RendererAsteroids.h"


SceneAsteroids::SceneAsteroids()
{
	sceneSettings.enableSkybox = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 200.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 500.0f;

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();

	SetWireframeEnabled(false);
}

void SceneAsteroids::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_2/right.png");
	skyboxFaces.push_back("Textures/skybox_2/left.png");
	skyboxFaces.push_back("Textures/skybox_2/top.png");
	skyboxFaces.push_back("Textures/skybox_2/bottom.png");
	skyboxFaces.push_back("Textures/skybox_2/back.png");
	skyboxFaces.push_back("Textures/skybox_2/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneAsteroids::SetTextures()
{
}

void SceneAsteroids::SetupMeshes()
{
}

void SceneAsteroids::SetupModels()
{
	ModelJoey* rock = new ModelJoey("Models/rock.obj");
	ModelJoey* planet = new ModelJoey("Models/planet.obj");
	models.insert(std::make_pair("rock", rock));
	models.insert(std::make_pair("planet", planet));
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
	for (auto& model : models)
		delete model.second;
}
