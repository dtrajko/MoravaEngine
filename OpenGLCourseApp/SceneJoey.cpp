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

	SetTextures();
	SetupModels();
	SetupGeometry();
	SetupLights();

	m_MaterialWorkflowPBR = new MaterialWorkflowPBR();
	m_MaterialWorkflowPBR->Init("Textures/HDR/Ice_Lake_Ref.hdr");
}

void SceneJoey::SetTextures()
{
	// rusted iron
	TextureInfo textureInfoRustedIron = {};
	textureInfoRustedIron.albedo = "Textures/PBR/rusted_iron/albedo.png";
	textureInfoRustedIron.normal = "Textures/PBR/rusted_iron/normal.png";
	textureInfoRustedIron.metallic = "Textures/PBR/rusted_iron/metallic.png";
	textureInfoRustedIron.roughness = "Textures/PBR/rusted_iron/roughness.png";
	textureInfoRustedIron.ao = "Textures/PBR/rusted_iron/ao.png";
	m_Materials.insert(std::make_pair("rusted_iron", new Material(textureInfoRustedIron)));

	// gold
	TextureInfo textureInfoGold = {};
	textureInfoGold.albedo = "Textures/PBR/gold/albedo.png";
	textureInfoGold.normal = "Textures/PBR/gold/normal.png";
	textureInfoGold.metallic = "Textures/PBR/gold/metallic.png";
	textureInfoGold.roughness = "Textures/PBR/gold/roughness.png";
	textureInfoGold.ao = "Textures/PBR/gold/ao.png";
	m_Materials.insert(std::make_pair("gold", new Material(textureInfoGold)));

	// silver
	TextureInfo textureInfoSilver = {};
	textureInfoSilver.albedo = "Textures/PBR/silver/albedo.png";
	textureInfoSilver.normal = "Textures/PBR/silver/normal.png";
	textureInfoSilver.metallic = "Textures/PBR/silver/metallic.png";
	textureInfoSilver.roughness = "Textures/PBR/silver/roughness.png";
	textureInfoSilver.ao = "Textures/PBR/silver/ao.png";
	m_Materials.insert(std::make_pair("silver", new Material(textureInfoSilver)));

	// grass
	TextureInfo textureInfoGrass = {};
	textureInfoGrass.albedo = "Textures/PBR/grass/albedo.png";
	textureInfoGrass.normal = "Textures/PBR/grass/normal.png";
	textureInfoGrass.metallic = "Textures/PBR/grass/metallic.png";
	textureInfoGrass.roughness = "Textures/PBR/grass/roughness.png";
	textureInfoGrass.ao = "Textures/PBR/grass/ao.png";
	m_Materials.insert(std::make_pair("grass", new Material(textureInfoGrass)));

	// plastic
	TextureInfo textureInfoPlastic = {};
	textureInfoPlastic.albedo = "Textures/PBR/plastic/albedo.png";
	textureInfoPlastic.normal = "Textures/PBR/plastic/normal.png";
	textureInfoPlastic.metallic = "Textures/PBR/plastic/metallic.png";
	textureInfoPlastic.roughness = "Textures/PBR/plastic/roughness.png";
	textureInfoPlastic.ao = "Textures/PBR/plastic/ao.png";
	m_Materials.insert(std::make_pair("plastic", new Material(textureInfoPlastic)));

	// wall
	TextureInfo textureInfoWall = {};
	textureInfoWall.albedo = "Textures/PBR/wall/albedo.png";
	textureInfoWall.normal = "Textures/PBR/wall/normal.png";
	textureInfoWall.metallic = "Textures/PBR/wall/metallic.png";
	textureInfoWall.roughness = "Textures/PBR/wall/roughness.png";
	textureInfoWall.ao = "Textures/PBR/wall/ao.png";
	m_Materials.insert(std::make_pair("wall", new Material(textureInfoWall)));

	// Cerberus model PBR textures
	TextureInfo textureInfoCerberus = {};
	textureInfoCerberus.albedo = "Textures/PBR/Cerberus/Cerberus_A.tga";
	textureInfoCerberus.normal = "Textures/PBR/Cerberus/Cerberus_N.tga";
	textureInfoCerberus.metallic = "Textures/PBR/Cerberus/Cerberus_M.tga";
	textureInfoCerberus.roughness = "Textures/PBR/Cerberus/Cerberus_R.tga";
	textureInfoCerberus.ao = "Textures/PBR/Cerberus/Cerberus_AO.tga";
	m_Materials.insert(std::make_pair("cerberus", new Material(textureInfoCerberus)));

	// Khronos DamagedHelmet model PBR textures
	TextureInfo textureInfoDamagedHelmet = {};
	textureInfoDamagedHelmet.albedo = "Textures/PBR/DamagedHelmet/Default_albedo.jpg";
	textureInfoDamagedHelmet.normal = "Textures/PBR/DamagedHelmet/Default_normal.jpg";
	textureInfoDamagedHelmet.metallic = "Textures/PBR/DamagedHelmet/Default_metalRoughness.jpg";
	textureInfoDamagedHelmet.roughness = "Textures/PBR/DamagedHelmet/Default_emissive.jpg";
	textureInfoDamagedHelmet.ao = "Textures/PBR/DamagedHelmet/Default_AO.jpg";
	m_Materials.insert(std::make_pair("damaged_helmet", new Material(textureInfoDamagedHelmet)));

	// Khronos SciFiHelmet model PBR textures
	TextureInfo textureInfoSFHelmet = {};
	textureInfoSFHelmet.albedo = "Textures/PBR/SciFiHelmet/SciFiHelmet_BaseColor.png";
	textureInfoSFHelmet.normal = "Textures/PBR/SciFiHelmet/SciFiHelmet_Normal.png";
	textureInfoSFHelmet.metallic = "Textures/PBR/SciFiHelmet/SciFiHelmet_MetallicRoughness.png";
	textureInfoSFHelmet.roughness = "Textures/PBR/SciFiHelmet/SciFiHelmet_Emissive.png";
	textureInfoSFHelmet.ao = "Textures/PBR/SciFiHelmet/SciFiHelmet_AmbientOcclusion.png";
	m_Materials.insert(std::make_pair("sf_helmet", new Material(textureInfoSFHelmet)));
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

void SceneJoey::SetSkybox()
{
}

void SceneJoey::Update(float timestep, Window& mainWindow)
{
}

void SceneJoey::UpdateImGui(float timestep, Window& mainWindow, std::map<const char*, float> profilerResults)
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
	glm::mat4 model;

	/* Begin pbrShader */
	{
		// initialize static shader uniforms before rendering
		shaders["pbrShader"]->Bind();
		shaders["pbrShader"]->setMat4("projection", projectionMatrix);
		shaders["pbrShader"]->setMat4("view", m_Camera->CalculateViewMatrix());
		shaders["pbrShader"]->setVec3("camPos", m_Camera->GetPosition());

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
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["rusted_iron"]->BindTextures(3);
		m_SphereJoey->Render();

		// gold
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-3.0, 0.0, 2.0));
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["gold"]->BindTextures(3);
		m_SphereJoey->Render();

		// grass
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-1.0, 0.0, 2.0));
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["grass"]->BindTextures(3);
		m_SphereJoey->Render();

		// plastic
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(1.0, 0.0, 2.0));
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["plastic"]->BindTextures(3);
		m_SphereJoey->Render();

		// wall
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(3.0, 0.0, 2.0));
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["wall"]->BindTextures(3);
		m_SphereJoey->Render();

		// render light source (simply re-render sphere at light positions)
		// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
		// keeps the codeprint small.
		for (unsigned int i = 0; i < SCENE_JOEY_LIGHT_COUNT; ++i)
		{
			glm::vec3 newPos = m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = m_LightPositions[i];
			shaders["pbrShader"]->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
			shaders["pbrShader"]->setVec3("lightColors[" + std::to_string(i) + "]", m_LightColors[i]);
			model = glm::mat4(1.0f);
			model = glm::translate(model, newPos);
			model = glm::scale(model, glm::vec3(0.5f));
			shaders["pbrShader"]->setMat4("model", model);
			GetMaterials()["silver"]->BindTextures(3);
			m_SphereJoey->Render();
		}

		/* Cerberus model */
		// glBindTexture(GL_TEXTURE_2D, sceneJoey->GetMaterials()["gold"]->GetTextureAO()->GetID()); // textures["goldAOMap"]->GetID()
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, -10.0f, 25.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.1f));
		shaders["pbrShader"]->setMat4("model", model);
		GetMaterials()["cerberus"]->BindTextures(3);
		GetModels()["cerberus"]->RenderModelPBR();
	}
	/* End pbrShader */

	/* Begin pbrShaderMRE */
	{
		m_Timestep = m_IsRotating ? m_Timestep - 0.1f * m_RotationFactor : 0.0f;

		shaders["pbrShaderMRE"]->Bind();
		shaders["pbrShaderMRE"]->setMat4("projection", projectionMatrix);
		shaders["pbrShaderMRE"]->setMat4("view", m_Camera->CalculateViewMatrix());
		shaders["pbrShaderMRE"]->setVec3("camPos", m_Camera->GetPosition());
		shaders["pbrShaderMRE"]->setFloat("emissiveFactor",  m_EmissiveFactor);
		shaders["pbrShaderMRE"]->setFloat("metalnessFactor", m_MetalnessFactor);
		shaders["pbrShaderMRE"]->setFloat("roughnessFactor", m_RoughnessFactor);

		for (unsigned int i = 0; i < SCENE_JOEY_LIGHT_COUNT; ++i)
		{
			glm::vec3 newPos = m_LightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = m_LightPositions[i];
			shaders["pbrShaderMRE"]->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
			shaders["pbrShaderMRE"]->setVec3("lightColors[" + std::to_string(i) + "]", m_LightColors[i]);
		}

		/* Khronos DamagedHelmet model */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-10.0f, 15.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f + m_Timestep), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		shaders["pbrShaderMRE"]->setMat4("model", model);
		GetMaterials()["damaged_helmet"]->BindTextures(3);
		GetModels()["damagedHelmet"]->RenderModelPBR();

		/* Khronos SciFiHelmet model */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 15.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f - m_Timestep), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		shaders["pbrShaderMRE"]->setMat4("model", model);
		GetMaterials()["sf_helmet"]->BindTextures(3);
		GetModels()["sfHelmet"]->RenderModelPBR();
	}
	/* End pbrShaderMRE */

	/* Begin backgroundShader */
	{
		// render skybox (render as last to prevent overdraw)
		shaders["backgroundShader"]->Bind();
		shaders["backgroundShader"]->setMat4("projection", projectionMatrix);
		shaders["backgroundShader"]->setMat4("view", m_Camera->CalculateViewMatrix());

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetEnvironmentCubemap());
		// glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetIrradianceMap()); // display irradiance map
		// glBindTexture(GL_TEXTURE_CUBE_MAP, m_MaterialWorkflowPBR->GetPrefilterMap()); // display prefilter map
		m_MaterialWorkflowPBR->GetSkyboxCube()->Render();
	}
	/* End backgroundShader */
}

void SceneJoey::RenderWater(glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms)
{
}

SceneJoey::~SceneJoey()
{
}
