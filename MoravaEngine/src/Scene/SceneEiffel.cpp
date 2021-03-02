#include "Scene/SceneEiffel.h"

#include "../../ImGuiWrapper.h"
#include "../../MeshData.h"
#include "../../Tile2D.h"

#include "Renderer/Renderer.h"
#include "Shader/ShaderMain.h"


SceneEiffel::SceneEiffel()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = true;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 16.0f, 28.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);
	sceneSettings.pointLights[0].base.enabled = false;
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 6.0f;
	sceneSettings.pointLights[1].base.enabled = false;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 6.0f;
	sceneSettings.pointLights[2].base.enabled = false;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 6.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.6f;
	sceneSettings.waterWaveSpeed = 0.2f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneEiffel::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_3/right.png");
	skyboxFaces.push_back("Textures/skybox_3/left.png");
	skyboxFaces.push_back("Textures/skybox_3/top.png");
	skyboxFaces.push_back("Textures/skybox_3/bottom.png");
	skyboxFaces.push_back("Textures/skybox_3/back.png");
	skyboxFaces.push_back("Textures/skybox_3/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneEiffel::SetupTextures()
{
	textures.insert(std::make_pair("sponzaCeilDiffuse", TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaCeilNormal",  TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_ddn.tga", false, false)));
	textures.insert(std::make_pair("water",             TextureLoader::Get()->GetTexture("Textures/water.png", false, false)));
	textures.insert(std::make_pair("pyramid",           TextureLoader::Get()->GetTexture("Textures/pyramid.png", false, false)));
}

void SceneEiffel::SetupMeshes()
{
	Mesh* cube = new Mesh();
	cube->Create(&MeshData::vertices[0], &MeshData::indices[0], MeshData::vertexCount, MeshData::indexCount);
	meshes.insert(std::make_pair("cube", cube));

	Mesh* quad = new Mesh();
	quad->Create(&MeshData::quadVertices[0], &MeshData::quadIndices[0], MeshData::quadVertexCount, MeshData::quadIndexCount);
	meshes.insert(std::make_pair("quad", quad));

	Mesh* quadLarge = new Mesh();
	quadLarge->Create(&MeshData::floorVertices[0], &MeshData::floorIndices[0], MeshData::floorVertexCount, MeshData::floorIndexCount);
	meshes.insert(std::make_pair("quadLarge", quadLarge));

	Tile2D* m_Tile2D = new Tile2D();
	meshes.insert(std::make_pair("water", m_Tile2D));
}

void SceneEiffel::SetupModels()
{ 
	Model* eiffel = new Model();
	eiffel->LoadModel("Models/Eiffel_Tower.obj");
	models.insert(std::make_pair("eiffel", eiffel));

	Model* watchtower = new Model();
	watchtower->LoadModel("Models/wooden_watch_tower.obj");
	models.insert(std::make_pair("watchtower", watchtower));

	Model* cerberus = new Model();
	cerberus->LoadModel("Models/Cerberus/Cerberus_LP.FBX", "Models/Cerberus/Textures");
	models.insert(std::make_pair("cerberus", cerberus));
}

void SceneEiffel::Update(float timestep, Window* mainWindow)
{
	m_Camera->OnUpdate(timestep);
}

void SceneEiffel::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	ImGui::Begin("Shadow rotation");
	{
		// Shadow rotation
		m_LightDirection = sceneSettings.directionalLight.direction;
		m_LightColor = LightManager::directionalLight.GetColor();

		float lightRadius = abs(m_LightDirection.x);
		float lightAngle = timestep * sceneSettings.shadowSpeed;
		m_LightDirection.x = (float)cos(lightAngle) * lightRadius;
		m_LightDirection.z = (float)sin(lightAngle) * lightRadius;

		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Water level", &sceneSettings.waterHeight, 0.0f, 20.0f);
			ImGui::SliderFloat3("DirLight direction", glm::value_ptr(m_LightDirection), -100.0f, 100.0f);
			ImGui::ColorEdit3("DirLight Color", glm::value_ptr(m_LightColor));
		}

		m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
		LightManager::directionalLight.SetDirection(m_LightDirection);
		LightManager::directionalLight.SetColor(m_LightColor);
	}
	ImGui::End();

	ImGui::Begin("Framebuffers");
	{
		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImVec2 imageSize(96.0f, 96.0f);

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

void SceneEiffel::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];

	glm::mat4 model;

	/* Floor */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.2f, 0.0f));
	model = glm::rotate(model, glm::radians(6.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(3.0f));
	shaderMain->setMat4("model", model);
	textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	if (passType != "shadow")
		meshes["quadLarge"]->Render();

	/* Eiffel model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.0003f, 0.0003f, 0.0003f));
	shaderMain->setMat4("model", model);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["eiffel"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Watchtower model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -0.35f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	shaderMain->setMat4("model", model);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["watchtower"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Cerberus model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 10.0f, 10.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.05f));
	shaderMain->setMat4("model", model);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["cerberus"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Cube */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	shaderMain->setMat4("model", model);
	textures["pyramid"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	if (passType == "main")
	{
		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		shaderMain->setMat4("model", model);
		shaderMain->setInt("albedoMap", textureSlots["shadow"]);
		shaderMain->setInt("normalMap", textureSlots["shadow"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();

		/* Water reflection framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		shaderMain->setMat4("model", model);
		shaderMain->setInt("albedoMap", textureSlots["diffuse"]);
		shaderMain->setInt("normalMap", textureSlots["normal"]);
		m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();

		/* Water refraction framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(-5.0f, 1.0f, 5.0f * (9.0f / 16.0f)));
		shaderMain->setMat4("model", model);
		shaderMain->setInt("albedoMap", textureSlots["diffuse"]);
		shaderMain->setInt("normalMap", textureSlots["normal"]);
		m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

void SceneEiffel::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

	Shader* shaderWater = shaders["water"];

	/* Water Tile */
	shaderWater->Bind();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
	shaderWater->setMat4("model", model);
	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	textures["waterNormal"]->Bind(textureSlots["normal"]);
	shaderWater->setInt("reflectionTexture", textureSlots["reflection"]);
	shaderWater->setInt("refractionTexture", textureSlots["refraction"]);
	shaderWater->setInt("normalMap", textureSlots["normal"]);
	shaderWater->setInt("depthMap", textureSlots["depth"]);
	shaderWater->setInt("dudvMap", textureSlots["DuDv"]);

	shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
	shaderWater->setVec3("lightPosition", -m_LightDirection);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->Render();
}

SceneEiffel::~SceneEiffel()
{
}
