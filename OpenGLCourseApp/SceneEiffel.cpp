#include "SceneEiffel.h"

#include "ImGuiWrapper.h"

#include "MeshData.h"
#include "Tile2D.h"
#include "ShaderMain.h"
#include "Renderer.h"
#include "WaterManager.h"


SceneEiffel::SceneEiffel()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = false;
	sceneSettings.enableSpotLights   = false;
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
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.diffuseIntensity = 6.0f;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pointLights[1].base.diffuseIntensity = 6.0f;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[2].base.diffuseIntensity = 6.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.6f;
	sceneSettings.waterWaveSpeed = 0.05f;

	SetSkybox();
	SetTextures();
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

void SceneEiffel::SetTextures()
{
	textures.insert(std::make_pair("sponzaCeilDiffuse", TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_diff.tga")));
	textures.insert(std::make_pair("sponzaCeilNormal",  TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_ddn.tga")));
	textures.insert(std::make_pair("water",             TextureLoader::Get()->GetTexture("Textures/water.png")));
	textures.insert(std::make_pair("pyramid",           TextureLoader::Get()->GetTexture("Textures/pyramid.png")));
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

	float vertices[] = { -1, -1, -1, 1, 1, -1, 1, -1, -1, 1, 1, 1 };
	unsigned int indices[] = { 0, 1, 2, 3, 4, 5 };
	Tile2D* m_Tile2D = new Tile2D();
	m_Tile2D->Create(&vertices[0], &indices[0], 12, 6);
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
	cerberus->LoadModel("Models/Cerberus_LP.FBX", "Textures/PBR/Cerberus");
	models.insert(std::make_pair("cerberus", cerberus));
}

void SceneEiffel::Update(float timestep, Window& mainWindow)
{
}

void SceneEiffel::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
{
	// Shadow rotation
	m_LightDirection = sceneSettings.directionalLight.direction;
	m_LightColor = m_LightManager->directionalLight.GetColor();

	float lightRadius = abs(m_LightDirection.x);
	float lightAngle = timestep * sceneSettings.shadowSpeed;
	m_LightDirection.x = (float)cos(lightAngle) * lightRadius;
	m_LightDirection.z = (float)sin(lightAngle) * lightRadius;

	ImGui::SliderFloat("Water level", &sceneSettings.waterHeight, 0.0f, 20.0f);
	ImGui::SliderFloat3("DirLight direction", glm::value_ptr(m_LightDirection), -100.0f, 100.0f);
	ImGui::ColorEdit3("DirLight Color", glm::value_ptr(m_LightColor));

	m_WaterManager->SetWaterHeight(sceneSettings.waterHeight);
	m_LightManager->directionalLight.SetDirection(m_LightDirection);
	m_LightManager->directionalLight.SetColor(m_LightColor);
}

void SceneEiffel::Render(Window& mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
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
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
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
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["eiffel"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Watchtower model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -0.35f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["watchtower"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Cerberus model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 10.0f, 10.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.05f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["cerberus"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Cube */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
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
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaderMain->setInt("theTexture", textureSlots["shadow"]);
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
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaderMain->setInt("theTexture", textureSlots["diffuse"]);
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
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaderMain->setInt("theTexture", textureSlots["diffuse"]);
		shaderMain->setInt("normalMap", textureSlots["normal"]);
		m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

void SceneEiffel::RenderWater(glm::mat4 projectionMatrix, std::string passType,
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
	model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));

	shaderWater->Bind();

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	m_WaterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	m_WaterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	m_WaterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	textures["waterNormal"]->Bind(textureSlots["normal"]);
	shaderWater->setInt("theTexture", textureSlots["reflection"]);
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
