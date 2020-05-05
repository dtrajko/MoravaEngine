#include "SceneTerrain.h"

#include "ImGuiWrapper.h"

#include "Tile2D.h"
#include "Terrain.h"
#include "Renderer.h"


SceneTerrain::SceneTerrain()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 25.0f, 200.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 8.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 600.0f;
	sceneSettings.ambientIntensity = 0.6f;
	sceneSettings.diffuseIntensity = 1.2f;
	sceneSettings.lightDirection = glm::vec3(1.2f, -14.0f, 1.2f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 2.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_1_position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pLight_1_diffuseIntensity = 2.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pLight_2_diffuseIntensity = 2.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 1.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
}

void SceneTerrain::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneTerrain::SetTextures()
{
	textures.insert(std::make_pair("rock", new Texture("Textures/rock.png")));
}

void SceneTerrain::SetupMeshes()
{
	float vertices[] = { -1, -1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1 };
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->Create(&vertices[0], &indices[0], 12, 6);
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneTerrain::SetupModels()
{
	Terrain* terrain = new Terrain("Textures/island_flat.png", 4.0f, nullptr); // heightmap_island_8x6.png
	Mesh* mesh = new Mesh();
	mesh->Create(terrain->GetVertices(), terrain->GetIndices(), terrain->GetVertexCount(), terrain->GetIndexCount());
	meshes.insert(std::make_pair("terrain", mesh));
}

void SceneTerrain::Update(float timestep, Window& mainWindow)
{
	m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
}

void SceneTerrain::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
	glm::vec3 dirLightDirection = m_LightManager->directionalLight.GetDirection();
	glm::vec3 dirLightColor = m_LightManager->directionalLight.GetColor();

	ImGui::Begin("Scene Settings");
	ImGui::SliderFloat("Water level", &sceneSettings.waterHeight, -20.0f, 100.0f);
	ImGui::SliderFloat3("Terrain scale", glm::value_ptr(m_TerrainScale), -4.0f, 4.0f);
	ImGui::SliderFloat("Tiling Factor", &m_Tiling_Factor, 0.0f, 5.0f);
	ImGui::Separator();
	ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(dirLightDirection), -1.0f, 1.0f);
	ImGui::ColorEdit3("DirLight Color", glm::value_ptr(dirLightColor));
	ImGui::End();

	m_LightManager->directionalLight.SetDirection(dirLightDirection);
	m_LightManager->directionalLight.SetColor(dirLightColor);

}

void SceneTerrain::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	if (passType == "shadow") return;

	/* Island */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(m_TerrainScale));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	textures["rock"]->Bind(textureSlots["diffuse"]);
	// textures["colorMap"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	shaders["main"]->setFloat("tilingFactor", m_Tiling_Factor);
	meshes["terrain"]->Render();
}

void SceneTerrain::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

	Shader* shaderWater = shaders["water"];
	shaderWater->Bind();

	/* Water Tile */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(256.0f, 1.0f, 256.0f));

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	shaderWater->setInt("reflectionTexture", textureSlots["reflection"]);
	shaderWater->setInt("refractionTexture", textureSlots["refraction"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->Render();
}

SceneTerrain::~SceneTerrain()
{
}
