#include "SceneSponza.h"

#include "ImGuiWrapper.h"

#include "MeshData.h"
#include "Tile2D.h"
#include "Renderer.h"


SceneSponza::SceneSponza()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = true;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(-4.0f, 10.0f, -0.5f);
	sceneSettings.cameraStartYaw = 0.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.2f;
	sceneSettings.directionalLight.base.diffuseIntensity = 1.0f;
	sceneSettings.directionalLight.direction = glm::vec3(0.10f, -0.16f, 0.38f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[1].position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[2].position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 2.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 0.2f;
	sceneSettings.waterWaveSpeed = 0.02f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneSponza::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneSponza::SetupTextures()
{
}

void SceneSponza::SetupMeshes()
{
	Mesh* quad = new Mesh();
	quad->Create(&MeshData::quadVertices[0], &MeshData::quadIndices[0], MeshData::quadVertexCount, MeshData::quadIndexCount);
	meshes.insert(std::make_pair("quad", quad));

	float vertices[] = { -1, -1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1 };
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->Create(&vertices[0], &indices[0], 12, 6);
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneSponza::SetupModels()
{
	Model* sponza = new Model();
	sponza->LoadModel("Models/sponza.obj");
	models.insert(std::make_pair("sponza", sponza));
}

void SceneSponza::Update(float timestep, Window& mainWindow)
{
}

void SceneSponza::UpdateImGui(float timestep, Window& mainWindow)
{
	// Shadow rotation
	glm::vec3 lightDirection = LightManager::directionalLight.GetDirection();
	glm::vec3 lightColor = LightManager::directionalLight.GetColor();
	float lightRadius = abs(lightDirection.x);
	float lightAngle = timestep * sceneSettings.shadowSpeed;
	lightDirection.x = (float)cos(lightAngle) * lightRadius;
	lightDirection.z = (float)sin(lightAngle) * lightRadius;
	LightManager::directionalLight.SetDirection(lightDirection);

	glm::vec3 PL0_Position = LightManager::pointLights[0].GetPosition();
	glm::vec3 PL0_Color = LightManager::pointLights[0].GetColor();
	float PL0_AmbIntensity = LightManager::pointLights[0].GetAmbientIntensity();
	float PL0_DiffIntensity = LightManager::pointLights[0].GetDiffuseIntensity();
	glm::vec3 PL1_Position = LightManager::pointLights[1].GetPosition();
	glm::vec3 PL1_Color = LightManager::pointLights[1].GetColor();
	float PL1_AmbIntensity = LightManager::pointLights[1].GetAmbientIntensity();
	float PL1_DiffIntensity = LightManager::pointLights[1].GetDiffuseIntensity();
	glm::vec3 PL2_Position = LightManager::pointLights[2].GetPosition();
	glm::vec3 PL2_Color = LightManager::pointLights[2].GetColor();
	float PL2_AmbIntensity = LightManager::pointLights[2].GetAmbientIntensity();
	float PL2_DiffIntensity = LightManager::pointLights[2].GetDiffuseIntensity();

	ImGui::SliderFloat("Water level", &sceneSettings.waterHeight, 0.0f, 20.0f);

	ImGui::ColorEdit3("DirLight Color", glm::value_ptr(lightColor));
	ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(lightDirection), -100.0f, 100.0f);

	ImGui::ColorEdit3("PL0 Color", glm::value_ptr(PL0_Color));
	ImGui::SliderFloat3("PL0 Position", glm::value_ptr(PL0_Position), -20.0f, 20.0f);
	ImGui::SliderFloat("PL0 Amb Intensity", &PL0_AmbIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat("PL0 Diff Intensity", &PL0_DiffIntensity, -20.0f, 20.0f);

	ImGui::ColorEdit3("PL1 Color", glm::value_ptr(PL1_Color));
	ImGui::SliderFloat3("PL1 Position", glm::value_ptr(PL1_Position), -20.0f, 20.0f);
	ImGui::SliderFloat("PL1 Amb Intensity", &PL1_AmbIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat("PL1 Diff Intensity", &PL1_DiffIntensity, -20.0f, 20.0f);

	ImGui::ColorEdit3("PL2 Color", glm::value_ptr(PL2_Color));
	ImGui::SliderFloat3("PL2 Position", glm::value_ptr(PL2_Position), -20.0f, 20.0f);
	ImGui::SliderFloat("PL2 Amb Intensity", &PL2_AmbIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat("PL2 Diff Intensity", &PL2_DiffIntensity, -20.0f, 20.0f);

	m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
	LightManager::directionalLight.SetDirection(lightDirection);
	LightManager::directionalLight.SetColor(lightColor);
	LightManager::pointLights[0].SetPosition(PL0_Position);
	LightManager::pointLights[0].SetColor(PL0_Color);
	LightManager::pointLights[0].SetAmbientIntensity(PL0_AmbIntensity);
	LightManager::pointLights[0].SetDiffuseIntensity(PL0_DiffIntensity);
	LightManager::pointLights[1].SetPosition(PL1_Position);
	LightManager::pointLights[1].SetColor(PL1_Color);
	LightManager::pointLights[1].SetAmbientIntensity(PL1_AmbIntensity);
	LightManager::pointLights[1].SetDiffuseIntensity(PL1_DiffIntensity);
	LightManager::pointLights[2].SetPosition(PL2_Position);
	LightManager::pointLights[2].SetColor(PL2_Color);
	LightManager::pointLights[2].SetAmbientIntensity(PL2_AmbIntensity);
	LightManager::pointLights[2].SetDiffuseIntensity(PL2_DiffIntensity);
}

void SceneSponza::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	/* Sponza scene */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.008f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["sponza"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	if (passType == "main")
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 16.0f, -0.25f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.2f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));

		shaders["main"]->setInt("albedoMap", textureSlots["shadow"]);
		shaders["main"]->setInt("normalMap", textureSlots["shadow"]);

		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

void SceneSponza::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

	Shader* shaderWater = shaders["water"];

	/* Water Tile */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(12.0f, 0.0f, 8.0f));

	shaderWater->Bind();
	shaderWater->setMat4("model", model);

	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);

	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaderWater->setInt("reflectionTexture", textureSlots["reflection"]);
	shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->Render();
}

SceneSponza::~SceneSponza()
{
}
