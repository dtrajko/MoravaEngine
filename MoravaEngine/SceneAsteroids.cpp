#include "SceneAsteroids.h"

#include "ImGuiWrapper.h"

#include "RendererAsteroids.h"


SceneAsteroids::SceneAsteroids()
{
	sceneSettings.enableSkybox = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 30.0f, 220.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraStartPitch = -10.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 500.0f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();

	SetWireframeEnabled(false);
}

void SceneAsteroids::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_1/right.jpg");
	skyboxFaces.push_back("Textures/skybox_1/left.jpg");
	skyboxFaces.push_back("Textures/skybox_1/top.jpg");
	skyboxFaces.push_back("Textures/skybox_1/bottom.jpg");
	skyboxFaces.push_back("Textures/skybox_1/back.jpg");
	skyboxFaces.push_back("Textures/skybox_1/front.jpg");

	m_Skybox = new Skybox(skyboxFaces);
}

void SceneAsteroids::SetupTextures()
{
}

void SceneAsteroids::SetupMeshes()
{
}

void SceneAsteroids::SetupModels()
{
	ModelJoey* rock = new ModelJoey("Models/rock.obj", "Textures");
	ModelJoey* planet = new ModelJoey("Models/planet.obj", "Textures");
	models.insert(std::make_pair("rock", rock));
	models.insert(std::make_pair("planet", planet));
}

void SceneAsteroids::Update(float timestep, Window& mainWindow)
{
}

void SceneAsteroids::UpdateImGui(float timestep, Window& mainWindow)
{
	/**** Not in use, Phong lighting model not supported by shaders
	glm::vec3 dirLightDirection = m_LightManager->directionalLight.GetDirection();
	glm::vec3 dirLightColor = m_LightManager->directionalLight.GetColor();
	ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(dirLightDirection), -1.0f, 1.0f);
	ImGui::ColorEdit3("DirLight Color", glm::value_ptr(dirLightColor));
	m_LightManager->directionalLight.SetDirection(dirLightDirection);
	m_LightManager->directionalLight.SetColor(dirLightColor); ****/

	ImGui::Begin("Settings");
	ImGui::SliderFloat("FOV", &m_FOV, -60.0f, 180.0f);
	ImGui::End();
}

void SceneAsteroids::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
}

SceneAsteroids::~SceneAsteroids()
{
	for (auto& model : models)
		delete model.second;

	models.clear();
}
