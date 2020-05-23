#include "Scene.h"

#include "MeshData.h"
#include "Tile2D.h"
#include "TextureLoader.h"


SceneSettings Scene::sceneSettings;

Scene::Scene()
{
	sceneSettings.enableShadows      = false;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = false;
	sceneSettings.enableSpotLights   = false;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = false;
	sceneSettings.enableNormalMaps   = false;
	sceneSettings.enableCulling      = false;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 20.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraStartPitch = 0.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 200.0f;

	// Directional light
	sceneSettings.directionalLight.base.enabled = true;
	sceneSettings.directionalLight.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.directionalLight.base.ambientIntensity = 0.2f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	// Point lights
	sceneSettings.pointLights[0].base.enabled = true;
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[0].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[0].constant = 0.4f;
	sceneSettings.pointLights[0].linear   = 0.3f;
	sceneSettings.pointLights[0].exponent = 0.2f;

	sceneSettings.pointLights[1].base.enabled = true;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[1].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[1].constant = 0.4f;
	sceneSettings.pointLights[1].linear   = 0.3f;
	sceneSettings.pointLights[1].exponent = 0.2f;

	sceneSettings.pointLights[2].base.enabled = true;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[2].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[2].constant = 0.4f;
	sceneSettings.pointLights[2].linear   = 0.3f;
	sceneSettings.pointLights[2].exponent = 0.2f;

	sceneSettings.pointLights[3].base.enabled = true;
	sceneSettings.pointLights[3].base.color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pointLights[3].position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pointLights[3].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[3].base.diffuseIntensity = 1.0f;
	sceneSettings.pointLights[3].constant = 0.4f;
	sceneSettings.pointLights[3].linear   = 0.3f;
	sceneSettings.pointLights[3].exponent = 0.2f;

	// Spot lights
	sceneSettings.spotLights[0].base.base.enabled = true;
	sceneSettings.spotLights[0].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[0].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[0].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[0].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[0].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[0].base.constant = 0.4f;
	sceneSettings.spotLights[0].base.linear   = 0.3f;
	sceneSettings.spotLights[0].base.exponent = 0.2f;
	sceneSettings.spotLights[0].edge = 35.0f;

	sceneSettings.spotLights[1].base.base.enabled = true;
	sceneSettings.spotLights[1].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[1].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[1].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[1].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[1].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[1].base.constant = 0.4f;
	sceneSettings.spotLights[1].base.linear   = 0.3f;
	sceneSettings.spotLights[1].base.exponent = 0.2f;
	sceneSettings.spotLights[1].edge = 35.0f;

	sceneSettings.spotLights[2].base.base.enabled = true;
	sceneSettings.spotLights[2].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[2].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[2].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[2].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[2].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[2].base.constant = 0.4f;
	sceneSettings.spotLights[2].base.linear   = 0.3f;
	sceneSettings.spotLights[2].base.exponent = 0.2f;
	sceneSettings.spotLights[2].edge = 35.0f;

	sceneSettings.spotLights[3].base.base.enabled = true;
	sceneSettings.spotLights[3].base.base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.spotLights[3].base.position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.spotLights[3].direction = glm::vec3(0.0f, -1.0f, 0.0f);
	sceneSettings.spotLights[3].base.base.ambientIntensity = 1.0f;
	sceneSettings.spotLights[3].base.base.diffuseIntensity = 1.0f;
	sceneSettings.spotLights[3].base.constant = 0.4f;
	sceneSettings.spotLights[3].base.linear   = 0.3f;
	sceneSettings.spotLights[3].base.exponent = 0.2f;
	sceneSettings.spotLights[3].edge = 35.0f;

	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.waterHeight = 1.6f;
	sceneSettings.waterWaveSpeed = 0.005f;

	shadowMapWidth = 1024;
	shadowMapHeight = 1024;

	m_WireframeEnabled = false;

	SetLightManager();
	SetTextures();
	SetTextureSlots();
	SetupMaterials();
	SetupMeshes();
}

void Scene::SetTextureSlots()
{
	textureSlots.insert(std::make_pair("diffuse",    1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("shadow",     3));
	textureSlots.insert(std::make_pair("omniShadow", 4));
	textureSlots.insert(std::make_pair("reflection", 5));
	textureSlots.insert(std::make_pair("refraction", 6));
	textureSlots.insert(std::make_pair("depth",      7));
	textureSlots.insert(std::make_pair("DuDv",       8));

	// PBR
	textureSlots.insert(std::make_pair("albedo",     1));
	textureSlots.insert(std::make_pair("normal",     2));
	textureSlots.insert(std::make_pair("metallic",   3));
	textureSlots.insert(std::make_pair("roughness",  4));
	textureSlots.insert(std::make_pair("ao",         5));

	// environment cubemap
	textureSlots.insert(std::make_pair("equirectangularMap", 6));
	textureSlots.insert(std::make_pair("environmentMap", 7));
}

void Scene::SetupMaterials()
{
	materials.insert(std::make_pair("shiny",      new Material(1.0f, 128.0f)));
	materials.insert(std::make_pair("dull",       new Material(1.0f, 64.0f)));
	materials.insert(std::make_pair("superShiny", new Material(1.0f, 1024.0f)));
	materials.insert(std::make_pair("superDull",  new Material(1.0f, 16.0f)));
}

void Scene::SetupMeshes()
{
}

void Scene::SetTextures()
{
	textures.insert(std::make_pair("normalMapDefault", TextureLoader::Get()->GetTexture("Textures/normal_map_default.png")));
	textures.insert(std::make_pair("shadowMapDefault", TextureLoader::Get()->GetTexture("Textures/shadow_map_default.png")));
	textures.insert(std::make_pair("waterDuDv",        TextureLoader::Get()->GetTexture("Textures/water/waterDuDv.png")));
	textures.insert(std::make_pair("waterNormal",      TextureLoader::Get()->GetTexture("Textures/water/waterNormal.png")));
}

void Scene::SetCamera()
{
	m_Camera = new Camera(sceneSettings.cameraPosition, glm::vec3(0.0f, 1.0f, 0.0f),
		sceneSettings.cameraStartYaw, sceneSettings.cameraStartPitch, sceneSettings.cameraMoveSpeed, 0.1f);
}

void Scene::SetLightManager()
{
	m_LightManager = new LightManager(sceneSettings);
}

void Scene::SetWaterManager(int width, int height)
{
	// Water framebuffers
	m_WaterManager = new WaterManager(width, height, sceneSettings.waterHeight, sceneSettings.waterWaveSpeed);
}

Scene::~Scene()
{
	delete m_Skybox;
	delete m_Camera;
	delete m_LightManager;
	delete m_WaterManager;

	//	for (auto& texture : textures) // TextureLoader is now responsible for deallocating
	//		if (texture.second != nullptr)
	//			delete texture.second;

	TextureLoader::Get()->Clean();

	for (auto& mesh : meshes)
		delete mesh.second;

	for (auto& material : materials)
		delete material.second;

	for (auto& model : models)
		delete model.second;

	skyboxFaces.clear();
	textures.clear();
	textureSlots.clear();
	materials.clear();
	meshes.clear();
	models.clear();

}
