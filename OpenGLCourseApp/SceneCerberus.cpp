#include "SceneCerberus.h"
#include "Terrain.h"
#include "ShaderWater.h"
#include "Renderer.h"


SceneCerberus::SceneCerberus()
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
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;
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
	sceneSettings.waterHeight = 6.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	SetSkybox();
	SetTextures();
	SetupModels();
}

void SceneCerberus::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_4/right.png");
	skyboxFaces.push_back("Textures/skybox_4/left.png");
	skyboxFaces.push_back("Textures/skybox_4/top.png");
	skyboxFaces.push_back("Textures/skybox_4/bottom.png");
	skyboxFaces.push_back("Textures/skybox_4/back.png");
	skyboxFaces.push_back("Textures/skybox_4/front.png");
	skybox = new Skybox(skyboxFaces);
}

void SceneCerberus::SetTextures()
{
}

void SceneCerberus::SetupModels()
{
	Model* cerberus = new Model();
	cerberus->LoadModel("Models/Cerberus/Cerberus_LP.obj");
	models.insert(std::make_pair("cerberus", cerberus));
}

void SceneCerberus::Update(float timestep, LightManager& lightManager, WaterManager* waterManager)
{
}

void SceneCerberus::Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
{
	Renderer::EnableCulling();
}

void SceneCerberus::RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
{
	if (!sceneSettings.enableWaterEffects) return;

	Renderer::EnableCulling();

	ShaderWater* shaderWater = (ShaderWater*)shaders["water"];

	shaderWater->Bind();

	/* Cerberus model */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 5.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["cerberus"]->RenderModel(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	/* Water Tile */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, waterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(256.0f, 1.0f, 256.0f));

	shaders["water"]->Bind();

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	waterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	waterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	shaders["water"]->SetTexture(textureSlots["reflection"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaderWater->SetLightColor(LightManager::directionalLight.GetColor());
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->RenderMesh();
}

SceneCerberus::~SceneCerberus()
{
}
