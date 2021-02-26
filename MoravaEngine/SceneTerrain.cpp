#include "SceneTerrain.h"

#include "ImGuiWrapper.h"
#include "TerrainHeightMap.h"
#include "Tile2D.h"
#include "src/Renderer/Renderer.h"


SceneTerrain::SceneTerrain()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 125.0f, 300.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 8.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 600.0f;

	sceneSettings.directionalLight.base.ambientIntensity = 0.6f;
	sceneSettings.directionalLight.base.diffuseIntensity = 1.2f;
	sceneSettings.directionalLight.direction = glm::vec3(1.2f, -14.0f, 1.2f);
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

	sceneSettings.shadowMapWidth = 1024;
	sceneSettings.shadowMapHeight = 1024;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 50.0f;
	sceneSettings.waterWaveSpeed = 0.2f;


	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();

	m_TerrainScale = glm::vec3(4.0f, 2.5f, 4.0f);
	m_Tiling_Factor = 1.0f;
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

void SceneTerrain::SetupTextures()
{
	textures.insert(std::make_pair("rock", TextureLoader::Get()->GetTexture("Textures/rock.png", false, false)));
}

void SceneTerrain::SetupMeshes()
{
	Tile2D* m_Tile2D = new Tile2D();
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneTerrain::SetupModels()
{
	TerrainHeightMap* terrain = new TerrainHeightMap("Textures/Noise/noise_001.png", 4.0f, nullptr); // heightmap_island_8x6.png
	meshes.insert(std::make_pair("terrain", terrain));
}

void SceneTerrain::Update(float timestep, Window* mainWindow)
{
	m_Camera->OnUpdate(timestep);
	m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
}

void SceneTerrain::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	glm::vec3 dirLightDirection = LightManager::directionalLight.GetDirection();
	glm::vec3 dirLightColor = LightManager::directionalLight.GetColor();

	ImGui::Begin("Scene Settings");
	{
		ImGui::SliderFloat("Water level", &sceneSettings.waterHeight, -20.0f, 100.0f);
		ImGui::SliderFloat3("Terrain scale", glm::value_ptr(m_TerrainScale), -4.0f, 4.0f);
		ImGui::SliderFloat("Tiling Factor", &m_Tiling_Factor, 0.0f, 5.0f);
		ImGui::Separator();
		ImGui::SliderFloat3("DirLight Direction", glm::value_ptr(dirLightDirection), -1.0f, 1.0f);
		ImGui::ColorEdit3("DirLight Color", glm::value_ptr(dirLightColor));
	}
	ImGui::End();

	LightManager::directionalLight.SetDirection(dirLightDirection);
	LightManager::directionalLight.SetColor(dirLightColor);

	ImGui::Begin("Framebuffers");
	{
		if (ImGui::CollapsingHeader("Display Info"))
		{
			ImVec2 imageSize(128.0f, 128.0f);

			ImGui::Text("Shadow Map");
			ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);

			if (ImGui::CollapsingHeader("Omni Shadow Maps"))
			{
				ImGui::Text("Omni Shadow Map 0\n(Point Light 0)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[0].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 1\n(Point Light 1)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[1].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 2\n(Point Light 2)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[2].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 3\n(Point Light 3)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[3].GetShadowMap()->GetTextureID(), imageSize);

				ImGui::Text("Omni Shadow Map 4\n(Spot Light 0)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[0].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 5\n(Spot Light 1)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[1].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 6\n(Spot Light 2)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[2].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 7\n(Spot Light 3)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[3].GetShadowMap()->GetTextureID(), imageSize);
			}

			ImGui::Text("Water Reflection\nColor Attachment");
			ImGui::Image((void*)(intptr_t)m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
			ImGui::Text("Water Refraction\nColor Attachment");
			ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->GetID(), imageSize);
			ImGui::Text("Water Refraction\nDepth Attachment");
			ImGui::Image((void*)(intptr_t)m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->GetID(), imageSize);
		}
	}
	ImGui::End();
}

void SceneTerrain::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	if (passType == "shadow") return;

	/* Island */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(m_TerrainScale));
	shaders["main"]->setMat4("model", model);
	materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	textures["rock"]->Bind(textureSlots["diffuse"]);
	// textures["colorMap"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	shaders["main"]->setFloat("tilingFactor", m_Tiling_Factor);
	meshes["terrain"]->Render();
}

void SceneTerrain::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
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

	shaderWater->setMat4("model", model);
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
