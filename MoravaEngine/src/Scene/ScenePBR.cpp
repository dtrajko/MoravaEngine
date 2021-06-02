#include "Scene/ScenePBR.h"

#include "Mesh/Sphere.h"
#include "Renderer/Renderer.h"
#include "Shader/ShaderMain.h"
#include "Shader/ShaderPBR.h"
#include "Terrain/TerrainHeightMap.h"


ScenePBR::ScenePBR()
{
	sceneSettings.enableShadows = true;
	sceneSettings.enableOmniShadows = false;
	sceneSettings.enablePointLights = true;
	sceneSettings.enableSpotLights = true;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox = true;
	sceneSettings.enableNormalMaps = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 10.0f, 15.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;

	sceneSettings.directionalLight.base.ambientIntensity = 0.02f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(1.2f, -14.0f, 1.2f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);

	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 0.0f;

	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[1].position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 0.0f;

	sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[2].position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 0.0f;

	sceneSettings.spotLights[2].base.base.color = glm::vec3(0.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[2].direction = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[2].base.base.ambientIntensity = 0.4f;
	sceneSettings.spotLights[2].base.base.diffuseIntensity = 4.0f;

	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 0.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	SetSkybox();
	SetupTextures();
	SetupMaterials();
	SetupModels();

	m_CameraPosition = glm::vec3(-9.0f, 24.0f, 6.0f);

	m_Albedo = glm::vec3(1.0f, 1.0f, 1.0f);
	m_Metallic = 0.8f;
	m_Roughness = 0.2f;
	m_AmbientOcclusion = 1.0f;
	m_AmbientIntensity = 0.5f;

	m_LightColor_0 = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColor_1 = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColor_2 = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColor_3 = glm::vec3(1.0f, 1.0f, 1.0f);

	m_LightPosOffset_0 = glm::vec3(-1.0f, 0.0f, 0.0f);
	m_LightPosOffset_1 = glm::vec3(1.0f, 0.0f, 0.0f);
	m_LightPosOffset_2 = glm::vec3(0.0f, -1.0f, 0.0f);
	m_LightPosOffset_3 = glm::vec3(0.0f, 1.0f, 0.0f);

	m_LightPositions[0] = m_CameraPosition + m_LightPosOffset_0;
	m_LightPositions[1] = m_CameraPosition + m_LightPosOffset_1;
	m_LightPositions[2] = m_CameraPosition + m_LightPosOffset_2;
	m_LightPositions[3] = m_CameraPosition + m_LightPosOffset_3;

	m_LightColors[0] = m_LightColor_0 * 255.0f;
	m_LightColors[1] = m_LightColor_1 * 255.0f;
	m_LightColors[2] = m_LightColor_2 * 255.0f;
	m_LightColors[3] = m_LightColor_3 * 255.0f;
}

void ScenePBR::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	m_Skybox = new Skybox(skyboxFaces);
}

void ScenePBR::SetupTextures()
{
	// PBR sphere
	textures.insert(std::make_pair("rustedIronAlbedo",           TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/albedo.png", false, false)));
	textures.insert(std::make_pair("rustedIronNormal",           TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/normal.png", false, false)));
	textures.insert(std::make_pair("rustedIronMetallic",         TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/metallic.png", false, false)));
	textures.insert(std::make_pair("rustedIronRoughness",        TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/roughness.png", false, false)));
	textures.insert(std::make_pair("rustedIronAmbientOcclusion", TextureLoader::Get()->GetTexture("Textures/PBR/rusted_iron/ao.png", false, false)));

	// PBR gold
	textures.insert(std::make_pair("goldAlbedo",           TextureLoader::Get()->GetTexture("Textures/PBR/gold/albedo.png", false, false)));
	textures.insert(std::make_pair("goldNormal",           TextureLoader::Get()->GetTexture("Textures/PBR/gold/normal.png", false, false)));
	textures.insert(std::make_pair("goldMetallic",         TextureLoader::Get()->GetTexture("Textures/PBR/gold/metallic.png", false, false)));
	textures.insert(std::make_pair("goldRoughness",        TextureLoader::Get()->GetTexture("Textures/PBR/gold/roughness.png", false, false)));
	textures.insert(std::make_pair("goldAmbientOcclusion", TextureLoader::Get()->GetTexture("Textures/PBR/gold/ao.png", false, false)));

	// PBR cerberus
	if (m_CerberusEnabled)
	{
		textures.insert(std::make_pair("cerberusAlbedo",       TextureLoader::Get()->GetTexture("Models/Cerberus/Textures/Cerberus_A.tga", false, false)));
		textures.insert(std::make_pair("cerberusNormal",       TextureLoader::Get()->GetTexture("Models/Cerberus/Textures/Cerberus_N.tga", false, false)));
		textures.insert(std::make_pair("cerberusMetallic",     TextureLoader::Get()->GetTexture("Models/Cerberus/Textures/Cerberus_M.tga", false, false)));
		textures.insert(std::make_pair("cerberusRoughness",    TextureLoader::Get()->GetTexture("Models/Cerberus/Textures/Cerberus_R.tga", false, false)));
		textures.insert(std::make_pair("cerberusAmbOcclusion", TextureLoader::Get()->GetTexture("Models/Cerberus/Textures/Cerberus_AO.tga", false, false)));
	}
}

void ScenePBR::SetupMaterials()
{
	float materialSpecular = 1.0f;
	float materialShininess = 256.0f;

	// silver
	TextureInfo textureInfoSilver = {};
	textureInfoSilver.albedo = "Textures/PBR/silver/albedo.png";
	textureInfoSilver.normal = "Textures/PBR/silver/normal.png";
	textureInfoSilver.metallic = "Textures/PBR/silver/metallic.png";
	textureInfoSilver.roughness = "Textures/PBR/silver/roughness.png";
	textureInfoSilver.ao = "Textures/PBR/silver/ao.png";

	materials.insert(std::make_pair("silver", new Material(textureInfoSilver, materialSpecular, materialShininess)));
}

void ScenePBR::SetupModels()
{
	Sphere* sphere = new Sphere();
	meshes.insert(std::make_pair("sphere", sphere));

	if (m_CerberusEnabled)
	{
		Model* cerberus = new Model();
		cerberus->LoadModel("Models/Cerberus/Cerberus_LP.FBX", "Models/Cerberus/Textures");
		models.insert(std::make_pair("cerberus", cerberus));
	}
}

void ScenePBR::Update(float timestep, Window* mainWindow)
{
	Scene::Update(timestep, mainWindow);
}

void ScenePBR::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	ImGui::Begin("Settings");
	{
		m_CameraPosition = m_Camera->GetPosition();

		ImGui::ColorEdit3("Albedo", glm::value_ptr(m_Albedo));
		ImGui::SliderFloat("Metallic", &m_Metallic, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &m_Roughness, 0.0f, 1.0f);
		ImGui::SliderFloat("Ambient Occlusion", &m_AmbientOcclusion, 0.0f, 5.0f);

		ImGui::SliderFloat("Ambient Intensity", &m_AmbientIntensity, 0.0f, 1.0f);

		ImGui::ColorEdit3("Light Color 0", glm::value_ptr(m_LightColor_0));
		ImGui::SliderFloat3("Light Pos Offset 0", glm::value_ptr(m_LightPosOffset_0), -60.0f, 60.0f);

		ImGui::ColorEdit3("Light Color 1", glm::value_ptr(m_LightColor_1));
		ImGui::SliderFloat3("Light Pos Offset 1", glm::value_ptr(m_LightPosOffset_1), -60.0f, 60.0f);

		ImGui::ColorEdit3("Light Color 2", glm::value_ptr(m_LightColor_2));
		ImGui::SliderFloat3("Light Pos Offset 2", glm::value_ptr(m_LightPosOffset_2), -60.0f, 60.0f);

		ImGui::ColorEdit3("Light Color 3", glm::value_ptr(m_LightColor_3));
		ImGui::SliderFloat3("Light Pos Offset 3", glm::value_ptr(m_LightPosOffset_3), -60.0f, 60.0f);

		m_LightPositions[0] = m_CameraPosition + m_LightPosOffset_0;
		m_LightPositions[1] = m_CameraPosition + m_LightPosOffset_1;
		m_LightPositions[2] = m_CameraPosition + m_LightPosOffset_2;
		m_LightPositions[3] = m_CameraPosition + m_LightPosOffset_3;

		m_LightColors[0] = m_LightColor_0 * 255.0f;
		m_LightColors[1] = m_LightColor_1 * 255.0f;
		m_LightColors[2] = m_LightColor_2 * 255.0f;
		m_LightColors[3] = m_LightColor_3 * 255.0f;
	}
	ImGui::End();
}

void ScenePBR::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms)
{
	ShaderPBR* shaderPBR = static_cast<ShaderPBR*>(shaders["pbr"]);

	shaderPBR->Bind();

	shaderPBR->SetLightPositions(m_LightPositions, 4);
	shaderPBR->SetLightColors(m_LightColors, 4);

	// Model matrix
	glm::mat4 model;

	// Render spheres in 5 x 5 setup
	for (int v = -2; v <= 2; ++v)
	{
		for (int h = -2; h <= 2; ++h)
		{
			float x = (float)h * 2.5f;
			float y = (float)v * 2.5f + 8.0f;
			float z = (float)v * 2.5f;

			/* Sphere model */
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(x, 6.0f, z));
			model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(1.0f));
			glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));

			shaderPBR->setVec3("albedo", m_Albedo);
			shaderPBR->setFloat("metallic", ((float)v / 5.0f + m_Metallic) / 2.0f);
			shaderPBR->setFloat("roughness", ((float)h / 5.0f + m_Roughness) / 2.0f);
			shaderPBR->setFloat("ao", m_AmbientOcclusion);
			shaderPBR->setFloat("ambientIntensity", m_AmbientIntensity);

			if ((v + h) % 2 == 0)
			{
				textures["rustedIronAlbedo"]->Bind(textureSlots["albedo"]);
				textures["rustedIronNormal"]->Bind(textureSlots["normal"]);
				textures["rustedIronMetallic"]->Bind(textureSlots["metallic"]);
				textures["rustedIronRoughness"]->Bind(textureSlots["roughness"]);
				textures["rustedIronAmbientOcclusion"]->Bind(textureSlots["ao"]);
			}
			else
			{
				textures["goldAlbedo"]->Bind(textureSlots["albedo"]);
				textures["goldNormal"]->Bind(textureSlots["normal"]);
				textures["goldMetallic"]->Bind(textureSlots["metallic"]);
				textures["goldRoughness"]->Bind(textureSlots["roughness"]);
				textures["goldAmbientOcclusion"]->Bind(textureSlots["ao"]);
			}

			shaderPBR->setInt("albedoMap", textureSlots["albedo"]);
			shaderPBR->setInt("normalMap", textureSlots["normal"]);
			shaderPBR->setInt("metallicMap", textureSlots["metallic"]);
			shaderPBR->setInt("roughnessMap", textureSlots["roughness"]);
			shaderPBR->setInt("aoMap", textureSlots["ao"]);

			meshes["sphere"]->Render();
		}
	}

	// render light source (simply re-render sphere at light positions)
	// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
	// keeps the codeprint small.
	for (unsigned int i = 0; i < SCENE_PBR_LIGHT_COUNT; ++i)
	{
		glm::vec3 newPos = m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
		newPos = m_LightPositions[i];
		shaderPBR->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
		shaderPBR->setVec3("lightColors[" + std::to_string(i) + "]", m_LightColors[i]);
		model = glm::mat4(1.0f);
		model = glm::translate(model, newPos);
		model = glm::scale(model, glm::vec3(0.1f));
		shaderPBR->setMat4("model", model);
		materials["silver"]->BindTextures(textureSlots["albedo"]);
		meshes["sphere"]->Render();
	}

	/* Cerberus model */
	if (m_CerberusEnabled)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, 2.5f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.05f));

		shaderPBR->setMat4("model", model);

		shaderPBR->setVec3("albedo", m_Albedo);
		shaderPBR->setFloat("metallic", m_Metallic);
		shaderPBR->setFloat("roughness", m_Roughness);
		shaderPBR->setFloat("ao", m_AmbientOcclusion);
		shaderPBR->setFloat("ambientIntensity", m_AmbientIntensity);

		textures["cerberusAlbedo"]->Bind(textureSlots["albedo"]);
		textures["cerberusNormal"]->Bind(textureSlots["normal"]);
		textures["cerberusMetallic"]->Bind(textureSlots["metallic"]);
		textures["cerberusRoughness"]->Bind(textureSlots["roughness"]);
		textures["cerberusAmbOcclusion"]->Bind(textureSlots["ao"]);

		shaderPBR->setInt("albedoMap", textureSlots["albedo"]);
		shaderPBR->setInt("normalMap", textureSlots["normal"]);
		shaderPBR->setInt("metallicMap", textureSlots["metallic"]);
		shaderPBR->setInt("roughnessMap", textureSlots["roughness"]);
		shaderPBR->setInt("aoMap", textureSlots["ao"]);

		models["cerberus"]->RenderPBR();
	}
}

void ScenePBR::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, MoravaShader*> shaders, std::map<std::string, int> uniforms)
{
	if (!sceneSettings.enableWaterEffects) return;

	MoravaShader* shaderWater = shaders["water"];

	// Model matrix
	glm::mat4 model;

	/* Water Tile */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, m_WaterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(32.0f, 1.0f, 32.0f));

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	shaderWater->setInt("reflectionTexture", textureSlots["reflection"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaderWater->setVec3("lightColor", LightManager::directionalLight.GetColor());
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->Render();
}

ScenePBR::~ScenePBR()
{
}
