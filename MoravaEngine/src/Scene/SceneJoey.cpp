#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneJoey.h"

#include "ImGui/ImGuiWrapper.h"
#include "Mesh/Sphere.h"
#include "Renderer/Renderer.h"
#include "Shader/ShaderMain.h"
#include "Shader/ShaderPBR.h"
#include "Terrain/TerrainHeightMap.h"


SceneJoey::SceneJoey()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = false;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = false;
	sceneSettings.enableSkybox       = false;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 8.0f, 30.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 1.0f;
	sceneSettings.nearPlane = 0.01f;
	sceneSettings.farPlane = 400.0f;

	sceneSettings.directionalLight.base.ambientIntensity = 0.4f;
	sceneSettings.directionalLight.base.diffuseIntensity = 1.0f;
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

	SetupShaders();
	SetupTextures();
	SetupMaterials();
	SetupModels();
	SetupGeometry();
	SetupLights();

	m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
	m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr");

	m_HDRI_Edit = HDRI_SAN_GIUSEPPE_BRIDGE;
	m_HDRI_Edit_Prev = m_HDRI_Edit;
	m_BlurLevel = 0;
	m_BlurLevelPrev = m_BlurLevel;
	m_SkyboxLOD = 0.0f;
}

void SceneJoey::SetupShaders()
{
	m_Shader_PBR = MoravaShader::Create("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.2.pbr.fs");
	printf("RendererJoey: m_Shader_PBR compiled [programID=%d]\n", m_Shader_PBR->GetProgramID());

	m_Shader_PBR_MRE = MoravaShader::Create("Shaders/LearnOpenGL/2.2.2.pbr.vs", "Shaders/LearnOpenGL/2.2.3.pbr.fs");
	printf("RendererJoey: m_Shader_PBR_MRE compiled [programID=%d]\n", m_Shader_PBR_MRE->GetProgramID());

	m_ShaderBackground = MoravaShader::Create("Shaders/LearnOpenGL/2.2.2.background.vs", "Shaders/LearnOpenGL/2.2.2.background.fs");
	printf("RendererJoey: m_ShaderBackground compiled [programID=%d]\n", m_ShaderBackground->GetProgramID());

	m_ShaderBlurHorizontal = MoravaShader::Create("Shaders/ThinMatrix/blur_horizontal.vs", "Shaders/ThinMatrix/blur.fs");
	printf("RendererJoey: m_ShaderBlurHorizontal compiled [programID=%d]\n", m_ShaderBlurHorizontal->GetProgramID());

	m_ShaderBlurVertical = MoravaShader::Create("Shaders/ThinMatrix/blur_vertical.vs", "Shaders/ThinMatrix/blur.fs");
	printf("RendererJoey: m_ShaderBlurVertical compiled [programID=%d]\n", m_ShaderBlurVertical->GetProgramID());

	m_Shader_PBR->Bind();
	m_Shader_PBR->SetInt("irradianceMap", 0);
	m_Shader_PBR->SetInt("prefilterMap", 1);
	m_Shader_PBR->SetInt("brdfLUT", 2);
	m_Shader_PBR->SetInt("albedoMap", 3);
	m_Shader_PBR->SetInt("normalMap", 4);
	m_Shader_PBR->SetInt("metallicMap", 5);
	m_Shader_PBR->SetInt("roughnessMap", 6);
	m_Shader_PBR->SetInt("aoMap", 7);

	m_Shader_PBR_MRE->Bind();
	m_Shader_PBR_MRE->SetInt("irradianceMap", 0);
	m_Shader_PBR_MRE->SetInt("prefilterMap", 1);
	m_Shader_PBR_MRE->SetInt("brdfLUT", 2);
	m_Shader_PBR_MRE->SetInt("albedoMap", 3);
	m_Shader_PBR_MRE->SetInt("normalMap", 4);
	m_Shader_PBR_MRE->SetInt("metalRoughMap", 5);
	m_Shader_PBR_MRE->SetInt("emissiveMap", 6);
	m_Shader_PBR_MRE->SetInt("aoMap", 7);

	m_ShaderBackground->Bind();
	m_ShaderBackground->SetInt("environmentMap", 0);
	m_ShaderBackground->SetFloat("u_TextureLOD", 0.0f);
}

void SceneJoey::SetupMaterials()
{
	// rusted iron
	TextureInfo textureInfoRustedIron = {};
	textureInfoRustedIron.albedo    = "Textures/PBR/rusted_iron/albedo.png";
	textureInfoRustedIron.normal    = "Textures/PBR/rusted_iron/normal.png";
	textureInfoRustedIron.metallic  = "Textures/PBR/rusted_iron/metallic.png";
	textureInfoRustedIron.roughness = "Textures/PBR/rusted_iron/roughness.png";
	textureInfoRustedIron.ao        = "Textures/PBR/rusted_iron/ao.png";
	materials.insert(std::make_pair("rusted_iron", new Material(textureInfoRustedIron, m_MaterialSpecular, m_MaterialShininess)));

	// gold
	TextureInfo textureInfoGold = {};
	textureInfoGold.albedo    = "Textures/PBR/gold/albedo.png";
	textureInfoGold.normal    = "Textures/PBR/gold/normal.png";
	textureInfoGold.metallic  = "Textures/PBR/gold/metallic.png";
	textureInfoGold.roughness = "Textures/PBR/gold/roughness.png";
	textureInfoGold.ao        = "Textures/PBR/gold/ao.png";
	materials.insert(std::make_pair("gold", new Material(textureInfoGold, m_MaterialSpecular, m_MaterialShininess)));

	// silver
	TextureInfo textureInfoSilver = {};
	textureInfoSilver.albedo    = "Textures/PBR/silver/albedo.png";
	textureInfoSilver.normal    = "Textures/PBR/silver/normal.png";
	textureInfoSilver.metallic  = "Textures/PBR/silver/metallic.png";
	textureInfoSilver.roughness = "Textures/PBR/silver/roughness.png";
	textureInfoSilver.ao        = "Textures/PBR/silver/ao.png";
	materials.insert(std::make_pair("silver", new Material(textureInfoSilver, m_MaterialSpecular, m_MaterialShininess)));

	// grass
	TextureInfo textureInfoGrass = {};
	textureInfoGrass.albedo    = "Textures/PBR/grass/albedo.png";
	textureInfoGrass.normal    = "Textures/PBR/grass/normal.png";
	textureInfoGrass.metallic  = "Textures/PBR/metalness.png";
	textureInfoGrass.roughness = "Textures/PBR/grass/roughness.png";
	textureInfoGrass.ao        = "Textures/PBR/grass/ao.png";
	materials.insert(std::make_pair("grass", new Material(textureInfoGrass, m_MaterialSpecular, m_MaterialShininess)));

	// plastic
	TextureInfo textureInfoPlastic = {};
	textureInfoPlastic.albedo    = "Textures/PBR/plastic/albedo.png";
	textureInfoPlastic.normal    = "Textures/PBR/plastic/normal.png";
	textureInfoPlastic.metallic  = "Textures/PBR/plastic/metallic.png";
	textureInfoPlastic.roughness = "Textures/PBR/plastic/roughness.png";
	textureInfoPlastic.ao        = "Textures/PBR/plastic/ao.png";
	materials.insert(std::make_pair("plastic", new Material(textureInfoPlastic, m_MaterialSpecular, m_MaterialShininess)));

	// wall
	TextureInfo textureInfoWall = {};
	textureInfoWall.albedo    = "Textures/PBR/wall/albedo.png";
	textureInfoWall.normal    = "Textures/PBR/wall/normal.png";
	textureInfoWall.metallic  = "Textures/PBR/metalness.png";
	textureInfoWall.roughness = "Textures/PBR/wall/roughness.png";
	textureInfoWall.ao        = "Textures/PBR/wall/ao.png";
	materials.insert(std::make_pair("wall", new Material(textureInfoWall, m_MaterialSpecular, m_MaterialShininess)));

	// Cerberus model PBR textures
	TextureInfo textureInfoCerberus = {};
	textureInfoCerberus.albedo    = "Models/Cerberus/Textures/Cerberus_A.tga";
	textureInfoCerberus.normal    = "Models/Cerberus/Textures/Cerberus_N.tga";
	textureInfoCerberus.metallic  = "Models/Cerberus/Textures/Cerberus_M.tga";
	textureInfoCerberus.roughness = "Models/Cerberus/Textures/Cerberus_R.tga";
	textureInfoCerberus.ao        = "Models/Cerberus/Textures/Cerberus_AO.tga";
	materials.insert(std::make_pair("cerberus", new Material(textureInfoCerberus, m_MaterialSpecular, m_MaterialShininess)));

	// Khronos DamagedHelmet model PBR textures
	TextureInfo textureInfoDamagedHelmet = {};
	textureInfoDamagedHelmet.albedo    = "Textures/PBR/DamagedHelmet/Default_albedo.jpg";
	textureInfoDamagedHelmet.normal    = "Textures/PBR/DamagedHelmet/Default_normal.jpg";
	textureInfoDamagedHelmet.metallic  = "Textures/PBR/DamagedHelmet/Default_metalRoughness.jpg";
	textureInfoDamagedHelmet.roughness = "Textures/PBR/DamagedHelmet/Default_emissive.jpg";
	textureInfoDamagedHelmet.ao        = "Textures/PBR/DamagedHelmet/Default_AO.jpg";
	materials.insert(std::make_pair("damaged_helmet", new Material(textureInfoDamagedHelmet, m_MaterialSpecular, m_MaterialShininess)));

	// Khronos SciFiHelmet model PBR textures
	TextureInfo textureInfoSFHelmet = {};
	textureInfoSFHelmet.albedo    = "Textures/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png";
	textureInfoSFHelmet.normal    = "Textures/PBR/SciFiHelmet/SciFiHelmet_Normal.png";
	textureInfoSFHelmet.metallic  = "Textures/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png";
	textureInfoSFHelmet.roughness = "Textures/PBR/SciFiHelmet/SciFiHelmet_Emissive.png";
	textureInfoSFHelmet.ao        = "Textures/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png";
	materials.insert(std::make_pair("sf_helmet", new Material(textureInfoSFHelmet, m_MaterialSpecular, m_MaterialShininess)));
}

void SceneJoey::SetupModels()
{
	Model* cerberus = new Model("Models/Cerberus/Cerberus_LP.FBX", "Models/Cerberus/Textures");
	models.insert(std::make_pair("cerberus", cerberus));

	Model* damagedHelmet = new Model("Models/DamagedHelmet.gltf", "Textures/PBR/DamagedHelmet");
	models.insert(std::make_pair("damagedHelmet", damagedHelmet));

	Model* sfHelmet = new Model("Models/SciFiHelmet.gltf", "Textures/PBR/SciFiHelmet");
	models.insert(std::make_pair("sfHelmet", sfHelmet));
}

void SceneJoey::SetupGeometry()
{
	m_SphereJoey = new SphereJoey();
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

void SceneJoey::Update(float timestep, Window* mainWindow)
{
	Scene::Update(timestep, mainWindow);

	if (m_HDRI_Edit != m_HDRI_Edit_Prev || m_BlurLevel != m_BlurLevelPrev)
	{
		if (m_HDRI_Edit == HDRI_GREENWICH_PARK) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/greenwich_park_02_1k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_SAN_GIUSEPPE_BRIDGE) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/san_giuseppe_bridge_1k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_TROPICAL_BEACH) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/Tropical_Beach_3k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_VIGNAIOLI_NIGHT) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/vignaioli_night_1k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_EARLY_EVE_WARM_SKY) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/006_hdrmaps_com_free.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_BIRCHWOOD) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/birchwood_4k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_PINK_SUNRISE) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/pink_sunrise_4k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_ROOITOU_PARK) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/rooitou_park_4k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_VENICE_DAWN) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/venice_dawn_1_4k.hdr", m_BlurLevel);
		}
		else if (m_HDRI_Edit == HDRI_PEPPERMINT_POWERPLANT) {
			m_MaterialWorkflowPBR->Init("Textures/HDR/peppermint_powerplant_1k.hdr", m_BlurLevel);
		}

		m_HDRI_Edit_Prev = m_HDRI_Edit;
		m_BlurLevelPrev = m_BlurLevel;
	}
}

void SceneJoey::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	if (m_ShowWindowCamera)
	{
		ImGui::Begin("Camera", &m_ShowWindowCamera);
		{
			if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
			{
				char buffer[100];
				sprintf(buffer, "Pitch         %.2f", m_Camera->GetPitch());
				ImGui::Text(buffer);
				sprintf(buffer, "Yaw           %.2f", m_Camera->GetYaw());
				ImGui::Text(buffer);
				sprintf(buffer, "FOV           %.2f", m_Camera->GetPerspectiveVerticalFOV());
				ImGui::Text(buffer);
				sprintf(buffer, "Aspect Ratio  %.2f", glm::degrees(m_Camera->GetAspectRatio()));
				ImGui::Text(buffer);
				sprintf(buffer, "Position    X %.2f Y %.2f Z %.2f", m_Camera->GetPosition().x, m_Camera->GetPosition().y, m_Camera->GetPosition().z);
				ImGui::Text(buffer);
				sprintf(buffer, "Direction   X %.2f Y %.2f Z %.2f", m_Camera->GetDirection().x, m_Camera->GetDirection().y, m_Camera->GetDirection().z);
				ImGui::Text(buffer);
				sprintf(buffer, "Front       X %.2f Y %.2f Z %.2f", m_Camera->GetFront().x, m_Camera->GetFront().y, m_Camera->GetFront().z);
				ImGui::Text(buffer);
				sprintf(buffer, "Up          X %.2f Y %.2f Z %.2f", m_Camera->GetUp().x, m_Camera->GetUp().y, m_Camera->GetUp().z);
				ImGui::Text(buffer);
				sprintf(buffer, "Right       X %.2f Y %.2f Z %.2f", m_Camera->GetRight().x, m_Camera->GetRight().y, m_Camera->GetRight().z);
				ImGui::Text(buffer);
			}
		}
		ImGui::End();
	}

	if (m_ShowWindowLights)
	{
		ImGui::Begin("Lights", &m_ShowWindowLights);
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

			m_LightPositions[0] = m_CameraPosition + m_LightPositionOffset[0];
			m_LightPositions[1] = m_CameraPosition + m_LightPositionOffset[1];
			m_LightPositions[2] = m_CameraPosition + m_LightPositionOffset[2];
			m_LightPositions[3] = m_CameraPosition + m_LightPositionOffset[3];

			m_LightColors[0] = m_LightColorsNormal[0] * 255.0f;
			m_LightColors[1] = m_LightColorsNormal[1] * 255.0f;
			m_LightColors[2] = m_LightColorsNormal[2] * 255.0f;
			m_LightColors[3] = m_LightColorsNormal[3] * 255.0f;
		}
		ImGui::End();
	}

	if (m_ShowWindowSelectHDRI)
	{
		ImGui::Begin("Select HDRI", &m_ShowWindowSelectHDRI);
		{
			ImGui::RadioButton("Greenwich Park", &m_HDRI_Edit, HDRI_GREENWICH_PARK);
			ImGui::RadioButton("San Giuseppe Bridge", &m_HDRI_Edit, HDRI_SAN_GIUSEPPE_BRIDGE);
			ImGui::RadioButton("Tropical Beach", &m_HDRI_Edit, HDRI_TROPICAL_BEACH);
			ImGui::RadioButton("Vignaioli Night", &m_HDRI_Edit, HDRI_VIGNAIOLI_NIGHT);
			ImGui::RadioButton("Early Eve & Warm Sky", &m_HDRI_Edit, HDRI_EARLY_EVE_WARM_SKY);
			ImGui::RadioButton("Birchwood", &m_HDRI_Edit, HDRI_BIRCHWOOD);
			ImGui::RadioButton("Pink Sunrise", &m_HDRI_Edit, HDRI_PINK_SUNRISE);
			ImGui::RadioButton("Rooitou Park", &m_HDRI_Edit, HDRI_ROOITOU_PARK);
			ImGui::RadioButton("Venice Dawn", &m_HDRI_Edit, HDRI_VENICE_DAWN);
			ImGui::RadioButton("Peppermint Powerplant", &m_HDRI_Edit, HDRI_PEPPERMINT_POWERPLANT);

			ImGui::SliderInt("Blur Level", &m_BlurLevel, 0, 10);
			ImGui::DragFloat("Skybox LOD", &m_SkyboxLOD, 0.01f, 0.0f, 6.0f, "%.2f");
		}
		ImGui::End();
	}

	if (m_ShowWindowMaterialTextures)
	{
		ImGui::Begin("Material Textures", &m_ShowWindowMaterialTextures);
		{
			ImVec2 imageSize(128.0f, 128.0f);

			ImGui::Text("BRDF LUT");
			ImGui::Image((void*)(intptr_t)m_MaterialWorkflowPBR->GetBRDF_LUT_Texture(), imageSize);
		}
		ImGui::End();
	}

}

// Demonstrate using DockSpace() to create an explicit docking node within an existing window.
// Note that you already dock windows into each others _without_ a DockSpace() by just moving windows 
// from their title bar (or by holding SHIFT if io.ConfigDockingWithShift is set).
// DockSpace() is only useful to construct to a central location for your application.
void SceneJoey::ShowExampleAppDockSpace(bool* p_open, Window* mainWindow)
{
	static bool opt_fullscreen_persistant = true;
	bool opt_fullscreen = opt_fullscreen_persistant;
	static ImGuiDockNodeFlags dockspace_flags =
		ImGuiDockNodeFlags_None |
		ImGuiDockNodeFlags_PassthruCentralNode |
		ImGuiDockNodeFlags_NoDockingInCentralNode;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	if (opt_fullscreen)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	}

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
	// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive, 
	// all active windows docked into it will lose their parent and become undocked.
	// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise 
	// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", p_open, window_flags);
	ImGui::PopStyleVar();

	if (opt_fullscreen)
	{
		ImGui::PopStyleVar(2);
	}

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else
	{
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("ERROR: Docking is not enabled! See Demo > Configuration.");
		ImGui::Text("Set io.ConfigFlags |= ImGuiConfigFlags_DockingEnable in your code, or ");
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::SmallButton("click here"))
		{
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		}
	}

	RenderImGuiMenu(mainWindow, dockspace_flags);

	ImGui::End();
}

void SceneJoey::RenderImGuiMenu(Window* mainWindow, ImGuiDockNodeFlags dockspaceFlags)
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit")) mainWindow->SetShouldClose(true);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::MenuItem("Undo");
			ImGui::MenuItem("Redo");
			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Camera"))
			{
				m_ShowWindowCamera = !m_ShowWindowCamera;
			}

			if (ImGui::MenuItem("Lights"))
			{
				m_ShowWindowLights = !m_ShowWindowLights;
			}

			if (ImGui::MenuItem("Select HDRI"))
			{
				m_ShowWindowSelectHDRI = !m_ShowWindowSelectHDRI;
			}

			if (ImGui::MenuItem("Material Textures"))
			{
				m_ShowWindowMaterialTextures = !m_ShowWindowMaterialTextures;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Docking"))
		{
			// Disabling fullscreen would allow the window to be moved to the front of other windows, 
			// which we can't undo at the moment without finer window depth/z control.
			//ImGui::MenuItem("Fullscreen", NULL, &opt_fullscreen_persistant);

			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspaceFlags & ImGuiDockNodeFlags_NoSplit) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoSplit;
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspaceFlags & ImGuiDockNodeFlags_NoResize) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoResize;
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_NoDockingInCentralNode;
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "", (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_PassthruCentralNode;
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspaceFlags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
				dockspaceFlags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::EndMenu();
		}

		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted("When docking is enabled, you can ALWAYS dock MOST window into another! Try it now!" "\n\n"
				" > if io.ConfigDockingWithShift==false (default):" "\n"
				"   drag windows from title bar to dock" "\n"
				" > if io.ConfigDockingWithShift==true:" "\n"
				"   drag windows from anywhere and hold Shift to dock" "\n\n"
				"This demo app has nothing to do with it!" "\n\n"
				"This demo app only demonstrate the use of ImGui::DockSpace() which allows you to manually create a docking node _within_ another window. This is useful so you can decorateyour main //   application window (e.g. with a menu bar)." "\n\n"
				"ImGui::DockSpace() comes with one hard constraint: it needs to be submitted _before_ any window which may be docked into it. Therefore, if you use a dock spot as the centralpoint of //  your application, you'll probably want it to be part of the very first window you are submitting to imgui every frame." "\n\n"
				"(NB: because of this constraint, the implicit \"Debug\" window can not be docked into an explicit DockSpace() node, because that window is submitted as part of the NewFrame( call.	An //easy workaround is that you can create your own implicit \"Debug##2\" window after calling DockSpace() and leave it in the window stack for anyone to use.)");
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}

		ImGui::EndMenuBar();
	}
}

void SceneJoey::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
	// configure global opengl state
	glEnable(GL_DEPTH_TEST);
	// set depth function to less than AND equal for skybox depth trick.
	glDepthFunc(GL_LEQUAL);
	// enable seamless cubemap sampling for lower mip levels in the pre-filter map.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// then before rendering, configure the viewport to the original framebuffer's screen dimensions
	RendererBasic::SetDefaultFramebuffer((unsigned int)mainWindow->GetWidth(), (unsigned int)mainWindow->GetHeight());

	glm::mat4 model;

	/* Begin pbrShader */
	{
		// initialize static shader uniforms before rendering
		m_Shader_PBR->Bind();
		m_Shader_PBR->SetMat4("projection", projectionMatrix);
		m_Shader_PBR->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shader_PBR->SetFloat3("camPos", m_Camera->GetPosition());

		// render scene, supplying the convoluted irradiance map to the final shader.
		// bind pre-computed IBL data
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, m_MaterialWorkflowPBR->GetBRDF_LUT_Texture());

		// rusted iron
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-5.0, 0.0, 2.0));
		m_Shader_PBR->SetMat4("model", model);
		materials["rusted_iron"]->BindTextures(3);
		m_SphereJoey->Render();

		// gold
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
		m_Shader_PBR->SetMat4("model", model);
		materials["gold"]->BindTextures(3);
		m_SphereJoey->Render();

		// grass
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0, 0.0, 2.0));
		m_Shader_PBR->SetMat4("model", model);
		materials["grass"]->BindTextures(3);
		m_SphereJoey->Render();

		// plastic
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(1.0, 0.0, 2.0));
		m_Shader_PBR->SetMat4("model", model);
		materials["plastic"]->BindTextures(3);
		m_SphereJoey->Render();

		// wall
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(3.0, 0.0, 2.0));
		m_Shader_PBR->SetMat4("model", model);
		materials["wall"]->BindTextures(3);
		m_SphereJoey->Render();

		// render light source (simply re-render sphere at light positions)
		// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
		// keeps the codeprint small.
		for (unsigned int i = 0; i < SCENE_JOEY_LIGHT_COUNT; ++i)
		{
			glm::vec3 newPos = m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = m_LightPositions[i];
			m_Shader_PBR->SetFloat3("lightPositions[" + std::to_string(i) + "]", newPos);
			m_Shader_PBR->SetFloat3("lightColors[" + std::to_string(i) + "]", m_LightColors[i]);
			model = glm::mat4(1.0f);
			model = glm::translate(model, newPos);
			model = glm::scale(model, glm::vec3(0.5f));
			m_Shader_PBR->SetMat4("model", model);
			materials["silver"]->BindTextures(3);
			m_SphereJoey->Render();
		}

		/* Cerberus model */
		// glBindTexture(GL_TEXTURE_2D, sceneJoey->materials["gold"]->GetTextureAO()->GetID()); // textures["goldAOMap"]->GetID()
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 5.0f, 20.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.1f));
		m_Shader_PBR->SetMat4("model", model);
		materials["cerberus"]->BindTextures(3);
		models["cerberus"]->RenderPBR();
	}
	/* End pbrShader */

	/* Begin pbrShaderMRE */
	{
		m_Timestep = m_IsRotating ? m_Timestep - 0.1f * m_RotationFactor : 0.0f;

		m_Shader_PBR_MRE->Bind();
		m_Shader_PBR_MRE->SetMat4("projection", projectionMatrix);
		m_Shader_PBR_MRE->SetMat4("view", m_Camera->GetViewMatrix());
		m_Shader_PBR_MRE->SetFloat3("camPos", m_Camera->GetPosition());
		m_Shader_PBR_MRE->SetFloat("emissiveFactor",  m_EmissiveFactor);
		m_Shader_PBR_MRE->SetFloat("metalnessFactor", m_MetalnessFactor);
		m_Shader_PBR_MRE->SetFloat("roughnessFactor", m_RoughnessFactor);

		for (unsigned int i = 0; i < SCENE_JOEY_LIGHT_COUNT; ++i)
		{
			glm::vec3 newPos = m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = m_LightPositions[i];
			m_Shader_PBR_MRE->SetFloat3("lightPositions[" + std::to_string(i) + "]", newPos);
			m_Shader_PBR_MRE->SetFloat3("lightColors[" + std::to_string(i) + "]", m_LightColors[i]);
		}

		/* Khronos DamagedHelmet model */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-10.0f, 15.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f + m_Timestep), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		m_Shader_PBR_MRE->SetMat4("model", model);
		materials["damaged_helmet"]->BindTextures(3);
		models["damagedHelmet"]->RenderPBR();

		/* Khronos SciFiHelmet model */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 15.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f - m_Timestep), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		m_Shader_PBR_MRE->SetMat4("model", model);
		materials["sf_helmet"]->BindTextures(3);
		models["sfHelmet"]->RenderPBR();
	}
	/* End pbrShaderMRE */

	/* Begin backgroundShader */
	{
		// render skybox (render as last to prevent overdraw)
		m_ShaderBackground->Bind();

		// Skybox shaderBackground
		RendererBasic::DisableCulling();
		// render skybox (render as last to prevent overdraw)

		glm::mat4 transform = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		// transform = glm::rotate(transform, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		m_ShaderBackground->SetMat4("model", transform);
		m_ShaderBackground->SetMat4("projection", projectionMatrix);
		m_ShaderBackground->SetMat4("view", m_Camera->GetViewMatrix());

		m_MaterialWorkflowPBR->BindEnvironmentCubemap(0);
		// m_MaterialWorkflowPBR->BindIrradianceMap(0); // display irradiance map
		// m_MaterialWorkflowPBR->BindPrefilterMap(0); // display prefilter map
		m_ShaderBackground->SetInt("environmentMap", 0);
		m_ShaderBackground->SetFloat("u_TextureLOD", m_SkyboxLOD);

		m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
	}
	/* End backgroundShader */
}

void SceneJoey::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, H2M::RefH2M<MoravaShader>> shaders, std::map<std::string, int> uniforms)
{
}
