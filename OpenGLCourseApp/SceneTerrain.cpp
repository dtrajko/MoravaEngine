#include "SceneTerrain.h"
#include "Renderer.h"
#include "Terrain.h"


SceneTerrain::SceneTerrain()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 12.0f, 60.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 5.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;
	sceneSettings.ambientIntensity = 0.2f;
	sceneSettings.diffuseIntensity = 1.0f;
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
	sceneSettings.waterWaveSpeed = 0.05f;

	SetSkybox();
	SetTextures();
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
	skybox = new Skybox(skyboxFaces);
}

void SceneTerrain::SetTextures()
{
	textures.insert(std::make_pair("rock", new Texture("Textures/rock.png")));
	textures["rock"]->LoadTexture();
}

void SceneTerrain::SetupModels()
{
	Terrain* terrain = new Terrain("Textures/heightmap_island_64x64.png", false); // heightmap_island_8x6.png
	Mesh* mesh = new Mesh();
	mesh->CreateMesh(terrain->GetVertices(), terrain->GetIndices(), terrain->GetVertexCount(), terrain->GetIndexCount());
	meshes.insert(std::make_pair("terrain", mesh));
}

void SceneTerrain::Update(float timestep, LightManager& lightManager)
{
}

void SceneTerrain::Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
{
	Renderer::DisableCulling();

	/* Island scene */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-32.0f, 0.0f, -32.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f, 0.1f, 1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	textures["rock"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	meshes["terrain"]->RenderMesh();
}

void SceneTerrain::RenderWater(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
{
	if (!sceneSettings.enableWaterEffects) return;

	Renderer::EnableCulling();

	/* Water Tile */
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, waterManager->GetWaterHeight(), 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(32.0f, 1.0f, 32.0f));

	shaders["water"]->Bind();

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	waterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["reflection"]);
	waterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["refraction"]);
	waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["depth"]);
	shaders["water"]->SetTexture(textureSlots["reflection"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	textures["waterDuDv"]->Bind(textureSlots["DuDv"]);
	shaders["water"]->SetLightColor(LightManager::directionalLight.GetColor());
	shaders["water"]->SetLightDirection(LightManager::directionalLight.GetDirection());
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["water"]->RenderMesh();
}

SceneTerrain::~SceneTerrain()
{
}
