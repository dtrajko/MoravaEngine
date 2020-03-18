#include "SceneJoey.h"
#include "Terrain.h"
#include "ShaderMain.h"
#include "ShaderWater.h"
#include "ShaderPBR.h"
#include "Renderer.h"
#include "Sphere.h"
#include "ImGuiWrapper.h"
#include "TextureJoey.h"


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
	// rusted iron
	textures.insert(std::make_pair("ironAlbedoMap",     new TextureJoey("Textures/PBR/rusted_iron/albedo.png")));
	textures.insert(std::make_pair("ironNormalMap",     new TextureJoey("Textures/PBR/rusted_iron/normal.png")));
	textures.insert(std::make_pair("ironMetallicMap",   new TextureJoey("Textures/PBR/rusted_iron/metallic.png")));
	textures.insert(std::make_pair("ironRoughnessMap",  new TextureJoey("Textures/PBR/rusted_iron/roughness.png")));
	textures.insert(std::make_pair("ironAOMap",         new TextureJoey("Textures/PBR/rusted_iron/ao.png")));

	// gold
	textures.insert(std::make_pair("goldAlbedoMap",     new TextureJoey("Textures/PBR/gold/albedo.png")));
	textures.insert(std::make_pair("goldNormalMap",     new TextureJoey("Textures/PBR/gold/normal.png")));
	textures.insert(std::make_pair("goldMetallicMap",   new TextureJoey("Textures/PBR/gold/metallic.png")));
	textures.insert(std::make_pair("goldRoughnessMap",  new TextureJoey("Textures/PBR/gold/roughness.png")));
	textures.insert(std::make_pair("goldAOMap",         new TextureJoey("Textures/PBR/gold/ao.png")));

	// grass
	textures.insert(std::make_pair("grassAlbedoMap",    new TextureJoey("Textures/PBR/grass/albedo.png")));
	textures.insert(std::make_pair("grassNormalMap",    new TextureJoey("Textures/PBR/grass/normal.png")));
	textures.insert(std::make_pair("grassMetallicMap",  new TextureJoey("Textures/PBR/grass/metallic.png")));
	textures.insert(std::make_pair("grassRoughnessMap", new TextureJoey("Textures/PBR/grass/roughness.png")));
	textures.insert(std::make_pair("grassAOMap",        new TextureJoey("Textures/PBR/grass/ao.png")));

	// plastic
	textures.insert(std::make_pair("plasticAlbedoMap",    new TextureJoey("Textures/PBR/plastic/albedo.png")));
	textures.insert(std::make_pair("plasticNormalMap",    new TextureJoey("Textures/PBR/plastic/normal.png")));
	textures.insert(std::make_pair("plasticMetallicMap",  new TextureJoey("Textures/PBR/plastic/metallic.png")));
	textures.insert(std::make_pair("plasticRoughnessMap", new TextureJoey("Textures/PBR/plastic/roughness.png")));
	textures.insert(std::make_pair("plasticAOMap",        new TextureJoey("Textures/PBR/plastic/ao.png")));

	// wall
	textures.insert(std::make_pair("wallAlbedoMap",     new TextureJoey("Textures/PBR/wall/albedo.png")));
	textures.insert(std::make_pair("wallNormalMap",     new TextureJoey("Textures/PBR/wall/normal.png")));
	textures.insert(std::make_pair("wallMetallicMap",   new TextureJoey("Textures/PBR/wall/metallic.png")));
	textures.insert(std::make_pair("wallRoughnessMap",  new TextureJoey("Textures/PBR/wall/roughness.png")));
	textures.insert(std::make_pair("wallAOMap",         new TextureJoey("Textures/PBR/wall/ao.png")));

	// Cerberus model PBR textures
	textures.insert(std::make_pair("cerberusAlbedoMap",       new TextureJoey("Textures/PBR/Cerberus/Cerberus_A.tga")));
	textures.insert(std::make_pair("cerberusNormalMap",       new TextureJoey("Textures/PBR/Cerberus/Cerberus_N.tga")));
	textures.insert(std::make_pair("cerberusMetallicMap",     new TextureJoey("Textures/PBR/Cerberus/Cerberus_M.tga")));
	textures.insert(std::make_pair("cerberusRoughnessMap",    new TextureJoey("Textures/PBR/Cerberus/Cerberus_R.tga")));
	textures.insert(std::make_pair("cerberusAmbOcclusionMap", new TextureJoey("Textures/PBR/Cerberus/Cerberus_AO.tga")));
}

void SceneJoey::SetupModels()
{
	Model* cerberus = new Model();
	cerberus->LoadModel("Models/Cerberus_LP.FBX", "Textures/PBR/Cerberus");
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
