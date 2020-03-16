#include "SceneJoey.h"
#include "Terrain.h"
#include "ShaderMain.h"
#include "ShaderWater.h"
#include "ShaderPBR.h"
#include "Renderer.h"
#include "Sphere.h"
#include "ImGuiWrapper.h"

#include "learnopengl/functions.h"


SceneJoey::SceneJoey()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = false;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 0.0f, 10.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;
	sceneSettings.ambientIntensity = 0.4f;
	sceneSettings.diffuseIntensity = 1.0f;
	sceneSettings.lightDirection = glm::vec3(1.2f, -14.0f, 1.2f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-36.0f, 36.0f, -36.0f, 36.0f, 0.1f, 36.0f);
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 0.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_1_position = glm::vec3(8.92f, 2.75f, -0.85f);
	sceneSettings.pLight_1_diffuseIntensity = 0.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pLight_2_diffuseIntensity = 0.0f;
	sceneSettings.sLight_2_color = glm::vec3(0.0f, 1.0f, 1.0f);
	sceneSettings.sLight_2_position = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.sLight_2_direction = glm::vec3(0.0f, 0.0f, 0.0f);
	sceneSettings.sLight_2_ambientIntensity = 0.4f;
	sceneSettings.sLight_2_diffuseIntensity = 4.0f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.1f;
	sceneSettings.waterHeight = 0.0f; // 1.0f 5.0f
	sceneSettings.waterWaveSpeed = 0.1f;

	SetTextures();
	SetupModels();

	// lights
	m_LightPositions[0] = glm::vec3(-10.0f,  10.0f, 10.0f);
	m_LightPositions[1] = glm::vec3( 10.0f,  10.0f, 10.0f);
	m_LightPositions[2] = glm::vec3(-10.0f, -10.0f, 10.0f);
	m_LightPositions[3] = glm::vec3( 10.0f, -10.0f, 10.0f);

	m_LightColors[0] = glm::vec3(300.0f, 300.0f, 300.0f);
	m_LightColors[1] = glm::vec3(300.0f, 300.0f, 300.0f);
	m_LightColors[2] = glm::vec3(300.0f, 300.0f, 300.0f);
	m_LightColors[3] = glm::vec3(300.0f, 300.0f, 300.0f);
}

void SceneJoey::SetSkybox()
{
}

void SceneJoey::SetTextures()
{
	// learnopengl textures
	// load PBR material textures

	// rusted iron
	unsigned int ironAlbedoMap = loadTexture("Textures/PBR/rusted_iron/albedo.png");
	unsigned int ironNormalMap = loadTexture("Textures/PBR/rusted_iron/normal.png");
	unsigned int ironMetallicMap = loadTexture("Textures/PBR/rusted_iron/metallic.png");
	unsigned int ironRoughnessMap = loadTexture("Textures/PBR/rusted_iron/roughness.png");
	unsigned int ironAOMap = loadTexture("Textures/PBR/rusted_iron/ao.png");

	m_TextureIDs.insert(std::make_pair("ironAlbedoMap",    ironAlbedoMap));
	m_TextureIDs.insert(std::make_pair("ironNormalMap",    ironNormalMap));
	m_TextureIDs.insert(std::make_pair("ironMetallicMap",  ironMetallicMap));
	m_TextureIDs.insert(std::make_pair("ironRoughnessMap", ironRoughnessMap));
	m_TextureIDs.insert(std::make_pair("ironAOMap",        ironAOMap));

	// gold
	unsigned int goldAlbedoMap = loadTexture("Textures/PBR/gold/albedo.png");
	unsigned int goldNormalMap = loadTexture("Textures/PBR/gold/normal.png");
	unsigned int goldMetallicMap = loadTexture("Textures/PBR/gold/metallic.png");
	unsigned int goldRoughnessMap = loadTexture("Textures/PBR/gold/roughness.png");
	unsigned int goldAOMap = loadTexture("Textures/PBR/gold/ao.png");

	m_TextureIDs.insert(std::make_pair("goldAlbedoMap",    goldAlbedoMap));
	m_TextureIDs.insert(std::make_pair("goldNormalMap",    goldNormalMap));
	m_TextureIDs.insert(std::make_pair("goldMetallicMap",  goldMetallicMap));
	m_TextureIDs.insert(std::make_pair("goldRoughnessMap", goldRoughnessMap));
	m_TextureIDs.insert(std::make_pair("goldAOMap",        goldAOMap));

	// grass
	unsigned int grassAlbedoMap = loadTexture("Textures/PBR/grass/albedo.png");
	unsigned int grassNormalMap = loadTexture("Textures/PBR/grass/normal.png");
	unsigned int grassMetallicMap = loadTexture("Textures/PBR/grass/metallic.png");
	unsigned int grassRoughnessMap = loadTexture("Textures/PBR/grass/roughness.png");
	unsigned int grassAOMap = loadTexture("Textures/PBR/grass/ao.png");

	m_TextureIDs.insert(std::make_pair("grassAlbedoMap",    grassAlbedoMap));
	m_TextureIDs.insert(std::make_pair("grassNormalMap",    grassNormalMap));
	m_TextureIDs.insert(std::make_pair("grassMetallicMap",  grassMetallicMap));
	m_TextureIDs.insert(std::make_pair("grassRoughnessMap", grassRoughnessMap));
	m_TextureIDs.insert(std::make_pair("grassAOMap",        grassAOMap));

	// plastic
	unsigned int plasticAlbedoMap = loadTexture("Textures/PBR/plastic/albedo.png");
	unsigned int plasticNormalMap = loadTexture("Textures/PBR/plastic/normal.png");
	unsigned int plasticMetallicMap = loadTexture("Textures/PBR/plastic/metallic.png");
	unsigned int plasticRoughnessMap = loadTexture("Textures/PBR/plastic/roughness.png");
	unsigned int plasticAOMap = loadTexture("Textures/PBR/plastic/ao.png");

	m_TextureIDs.insert(std::make_pair("plasticAlbedoMap",    plasticAlbedoMap));
	m_TextureIDs.insert(std::make_pair("plasticNormalMap",    plasticNormalMap));
	m_TextureIDs.insert(std::make_pair("plasticMetallicMap",  plasticMetallicMap));
	m_TextureIDs.insert(std::make_pair("plasticRoughnessMap", plasticRoughnessMap));
	m_TextureIDs.insert(std::make_pair("plasticAOMap",        plasticAOMap));

	// wall
	unsigned int wallAlbedoMap = loadTexture("Textures/PBR/wall/albedo.png");
	unsigned int wallNormalMap = loadTexture("Textures/PBR/wall/normal.png");
	unsigned int wallMetallicMap = loadTexture("Textures/PBR/wall/metallic.png");
	unsigned int wallRoughnessMap = loadTexture("Textures/PBR/wall/roughness.png");
	unsigned int wallAOMap = loadTexture("Textures/PBR/wall/ao.png");

	m_TextureIDs.insert(std::make_pair("wallAlbedoMap",    wallAlbedoMap));
	m_TextureIDs.insert(std::make_pair("wallNormalMap",    wallNormalMap));
	m_TextureIDs.insert(std::make_pair("wallMetallicMap",  wallMetallicMap));
	m_TextureIDs.insert(std::make_pair("wallRoughnessMap", wallRoughnessMap));
	m_TextureIDs.insert(std::make_pair("wallAOMap",        wallAOMap));

	// Cerberus model PBR textures
	textures.insert(std::make_pair("cerberusAlbedo", new Texture("Textures/PBR/Cerberus/Cerberus_A.tga")));
	textures.insert(std::make_pair("cerberusNormal", new Texture("Textures/PBR/Cerberus/Cerberus_N.tga")));
	textures.insert(std::make_pair("cerberusMetallic", new Texture("Textures/PBR/Cerberus/Cerberus_M.tga")));
	textures.insert(std::make_pair("cerberusRoughness", new Texture("Textures/PBR/Cerberus/Cerberus_R.tga")));
	textures.insert(std::make_pair("cerberusAmbientOcclusion", new Texture("Textures/PBR/Cerberus/Cerberus_AO.tga")));
	textures["cerberusAlbedo"]->Load();
	textures["cerberusNormal"]->Load();
	textures["cerberusMetallic"]->Load();
	textures["cerberusRoughness"]->Load();
	textures["cerberusAmbientOcclusion"]->Load();

	m_TextureIDs.insert(std::make_pair("cerberusAlbedoMap",    textures["cerberusAlbedo"]->GetID()));
	m_TextureIDs.insert(std::make_pair("cerberusNormalMap",    textures["cerberusNormal"]->GetID()));
	m_TextureIDs.insert(std::make_pair("cerberusMetallicMap",  textures["cerberusMetallic"]->GetID()));
	m_TextureIDs.insert(std::make_pair("cerberusRoughnessMap", textures["cerberusRoughness"]->GetID()));
	m_TextureIDs.insert(std::make_pair("cerberusAOMap",        textures["cerberusAmbientOcclusion"]->GetID()));
}

void SceneJoey::SetupModels()
{
	Model* cerberus = new Model();
	cerberus->LoadModel("Models/Cerberus_LP.FBX");
	models.insert(std::make_pair("cerberus", cerberus));
}

void SceneJoey::Update(float timestep)
{
}

void SceneJoey::Render(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

void SceneJoey::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneJoey::~SceneJoey()
{
}
