#include "SceneJoey.h"
#include "Terrain.h"
#include "ShaderMain.h"
#include "ShaderPBR.h"
#include "Renderer.h"
#include "Sphere.h"
#include "ImGuiWrapper.h"


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
	SetupLights();
}

void SceneJoey::SetupLights()
{
	m_CameraPosition = sceneSettings.cameraPosition;

	// lights
	m_LightColorsNormal[0] = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColorsNormal[1] = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColorsNormal[2] = glm::vec3(1.0f, 1.0f, 1.0f);
	m_LightColorsNormal[3] = glm::vec3(1.0f, 1.0f, 1.0f);

	m_LightPositionOffset[0] = glm::vec3(-3.0f, 10.0f, -3.0f);
	m_LightPositionOffset[1] = glm::vec3( 3.0f, 10.0f, -3.0f);
	m_LightPositionOffset[2] = glm::vec3(-3.0f, 10.0f,  3.0f);
	m_LightPositionOffset[3] = glm::vec3( 3.0f, 10.0f,  3.0f);

	m_LightPositions[0] = m_LightPositionOffset[0]; // m_CameraPosition + m_LightPositionOffset[0];
	m_LightPositions[1] = m_LightPositionOffset[1]; // m_CameraPosition + m_LightPositionOffset[1];
	m_LightPositions[2] = m_LightPositionOffset[2]; // m_CameraPosition + m_LightPositionOffset[2];
	m_LightPositions[3] = m_LightPositionOffset[3]; // m_CameraPosition + m_LightPositionOffset[3];

	m_LightColors[0] = m_LightColorsNormal[0] * 300.0f;
	m_LightColors[1] = m_LightColorsNormal[1] * 300.0f;
	m_LightColors[2] = m_LightColorsNormal[2] * 300.0f;
	m_LightColors[3] = m_LightColorsNormal[3] * 300.0f;

	m_EmissiveFactor = 4.0f;
	m_MetalnessFactor = 1.0f;
	m_RoughnessFactor = 1.0f;
	m_IsRotating = false;
	m_RotationFactor = 0.0f;
}

void SceneJoey::SetSkybox()
{
}

void SceneJoey::SetTextures()
{
	// rusted iron
	textures.insert(std::make_pair("ironAlbedoMap",     new Texture("Textures/PBR/rusted_iron/albedo.png")));
	textures.insert(std::make_pair("ironNormalMap",     new Texture("Textures/PBR/rusted_iron/normal.png")));
	textures.insert(std::make_pair("ironMetallicMap",   new Texture("Textures/PBR/rusted_iron/metallic.png")));
	textures.insert(std::make_pair("ironRoughnessMap",  new Texture("Textures/PBR/rusted_iron/roughness.png")));
	textures.insert(std::make_pair("ironAOMap",         new Texture("Textures/PBR/rusted_iron/ao.png")));

	// gold
	textures.insert(std::make_pair("goldAlbedoMap",     new Texture("Textures/PBR/gold/albedo.png")));
	textures.insert(std::make_pair("goldNormalMap",     new Texture("Textures/PBR/gold/normal.png")));
	textures.insert(std::make_pair("goldMetallicMap",   new Texture("Textures/PBR/gold/metallic.png")));
	textures.insert(std::make_pair("goldRoughnessMap",  new Texture("Textures/PBR/gold/roughness.png")));
	textures.insert(std::make_pair("goldAOMap",         new Texture("Textures/PBR/gold/ao.png")));

	// silver
	textures.insert(std::make_pair("silverAlbedoMap",    new Texture("Textures/PBR/silver/albedo.png")));
	textures.insert(std::make_pair("silverNormalMap",    new Texture("Textures/PBR/silver/normal.png")));
	textures.insert(std::make_pair("silverMetallicMap",  new Texture("Textures/PBR/silver/metallic.png")));
	textures.insert(std::make_pair("silverRoughnessMap", new Texture("Textures/PBR/silver/roughness.png")));
	textures.insert(std::make_pair("silverAOMap",        new Texture("Textures/PBR/silver/ao.png")));

	// grass
	textures.insert(std::make_pair("grassAlbedoMap",    new Texture("Textures/PBR/grass/albedo.png")));
	textures.insert(std::make_pair("grassNormalMap",    new Texture("Textures/PBR/grass/normal.png")));
	textures.insert(std::make_pair("grassMetallicMap",  new Texture("Textures/PBR/grass/metallic.png")));
	textures.insert(std::make_pair("grassRoughnessMap", new Texture("Textures/PBR/grass/roughness.png")));
	textures.insert(std::make_pair("grassAOMap",        new Texture("Textures/PBR/grass/ao.png")));

	// plastic
	textures.insert(std::make_pair("plasticAlbedoMap",    new Texture("Textures/PBR/plastic/albedo.png")));
	textures.insert(std::make_pair("plasticNormalMap",    new Texture("Textures/PBR/plastic/normal.png")));
	textures.insert(std::make_pair("plasticMetallicMap",  new Texture("Textures/PBR/plastic/metallic.png")));
	textures.insert(std::make_pair("plasticRoughnessMap", new Texture("Textures/PBR/plastic/roughness.png")));
	textures.insert(std::make_pair("plasticAOMap",        new Texture("Textures/PBR/plastic/ao.png")));

	// wall
	textures.insert(std::make_pair("wallAlbedoMap",     new Texture("Textures/PBR/wall/albedo.png")));
	textures.insert(std::make_pair("wallNormalMap",     new Texture("Textures/PBR/wall/normal.png")));
	textures.insert(std::make_pair("wallMetallicMap",   new Texture("Textures/PBR/wall/metallic.png")));
	textures.insert(std::make_pair("wallRoughnessMap",  new Texture("Textures/PBR/wall/roughness.png")));
	textures.insert(std::make_pair("wallAOMap",         new Texture("Textures/PBR/wall/ao.png")));

	// Cerberus model PBR textures
	textures.insert(std::make_pair("cerberusAlbedoMap",       new Texture("Textures/PBR/Cerberus/Cerberus_A.tga")));
	textures.insert(std::make_pair("cerberusNormalMap",       new Texture("Textures/PBR/Cerberus/Cerberus_N.tga")));
	textures.insert(std::make_pair("cerberusMetallicMap",     new Texture("Textures/PBR/Cerberus/Cerberus_M.tga")));
	textures.insert(std::make_pair("cerberusRoughnessMap",    new Texture("Textures/PBR/Cerberus/Cerberus_R.tga")));
	textures.insert(std::make_pair("cerberusAmbOcclusionMap", new Texture("Textures/PBR/Cerberus/Cerberus_AO.tga")));

	// Khronos DamagedHelmet model PBR textures
	textures.insert(std::make_pair("damagedHelmetAlbedoMap",       new Texture("Textures/PBR/DamagedHelmet/Default_albedo.jpg")));
	textures.insert(std::make_pair("damagedHelmetNormalMap",       new Texture("Textures/PBR/DamagedHelmet/Default_normal.jpg")));
	textures.insert(std::make_pair("damagedHelmetMetalRoughMap",   new Texture("Textures/PBR/DamagedHelmet/Default_metalRoughness.jpg")));
	textures.insert(std::make_pair("damagedHelmetEmissiveMap",     new Texture("Textures/PBR/DamagedHelmet/Default_emissive.jpg")));
	textures.insert(std::make_pair("damagedHelmetAmbOcclusionMap", new Texture("Textures/PBR/DamagedHelmet/Default_AO.jpg")));

	// Khronos SciFiHelmet model PBR textures
	textures.insert(std::make_pair("sfHelmetAlbedoMap",       new Texture("Textures/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png")));
	textures.insert(std::make_pair("sfHelmetNormalMap",       new Texture("Textures/PBR/SciFiHelmet/SciFiHelmet_Normal.png")));
	textures.insert(std::make_pair("sfHelmetMetalRoughMap",   new Texture("Textures/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png")));
	textures.insert(std::make_pair("sfHelmetEmissiveMap",     new Texture("Textures/PBR/SciFiHelmet/SciFiHelmet_Emissive.png")));
	textures.insert(std::make_pair("sfHelmetAmbOcclusionMap", new Texture("Textures/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png")));
}

void SceneJoey::SetupModels()
{
	Model* cerberus = new Model();
	cerberus->LoadModel("Models/Cerberus_LP.FBX", "Textures/PBR/Cerberus");
	models.insert(std::make_pair("cerberus", cerberus));

	Model* damagedHelmet = new Model();
	damagedHelmet->LoadModel("Models/DamagedHelmet.gltf", "Textures/PBR/DamagedHelmet");
	models.insert(std::make_pair("damagedHelmet", damagedHelmet));

	Model* sfHelmet = new Model();
	sfHelmet->LoadModel("Models/SciFiHelmet.gltf", "Textures/PBR/SciFiHelmet");
	models.insert(std::make_pair("sfHelmet", sfHelmet));
}

void SceneJoey::Update(float timestep, Window& mainWindow)
{
}

void SceneJoey::UpdateImGui(float timestep, Window& mainWindow)
{
	m_CameraPosition = m_Camera->GetPosition();

	ImGui::ColorEdit3("Light Color 0", glm::value_ptr(m_LightColorsNormal[0]));
	ImGui::SliderFloat3("Light Pos Offset 0", glm::value_ptr(m_LightPositionOffset[0]), -60.0f, 60.0f);

	ImGui::ColorEdit3("Light Color 1", glm::value_ptr(m_LightColorsNormal[1]));
	ImGui::SliderFloat3("Light Pos Offset 1", glm::value_ptr(m_LightPositionOffset[1]), -60.0f, 60.0f);

	ImGui::ColorEdit3("Light Color 2", glm::value_ptr(m_LightColorsNormal[2]));
	ImGui::SliderFloat3("Light Pos Offset 2", glm::value_ptr(m_LightPositionOffset[2]), -60.0f, 60.0f);

	ImGui::ColorEdit3("Light Color 3", glm::value_ptr(m_LightColorsNormal[3]));
	ImGui::SliderFloat3("Light Pos Offset 3", glm::value_ptr(m_LightPositionOffset[3]), -60.0f, 60.0f);

	ImGui::SliderFloat("Emissive Factor", &m_EmissiveFactor, 0.0f, 10.0f);
	ImGui::SliderFloat("Metalness Factor", &m_MetalnessFactor, 0.0f, 1.0f);
	ImGui::SliderFloat("Roughness Factor", &m_RoughnessFactor, 0.0f, 1.0f);

	ImGui::Checkbox("Is Rotating?", &m_IsRotating);
	ImGui::SliderFloat("Rotation Factor", &m_RotationFactor, 0.0f, 10.0f);

	m_LightPositions[0] = m_LightPositionOffset[0]; // m_CameraPosition + m_LightPositionOffset[0];
	m_LightPositions[1] = m_LightPositionOffset[1]; // m_CameraPosition + m_LightPositionOffset[1];
	m_LightPositions[2] = m_LightPositionOffset[2]; // m_CameraPosition + m_LightPositionOffset[2];
	m_LightPositions[3] = m_LightPositionOffset[3]; // m_CameraPosition + m_LightPositionOffset[3];

	m_LightColors[0] = m_LightColorsNormal[0] * 255.0f;
	m_LightColors[1] = m_LightColorsNormal[1] * 255.0f;
	m_LightColors[2] = m_LightColorsNormal[2] * 255.0f;
	m_LightColors[3] = m_LightColorsNormal[3] * 255.0f;
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
