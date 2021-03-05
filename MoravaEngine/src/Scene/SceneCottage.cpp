#define _CRT_SECURE_NO_WARNINGS

#include "Scene/SceneCottage.h"

#include "Mesh/MeshData.h"
#include "Mesh/Sphere.h"
#include "Shader/ShaderMain.h"


SceneCottage::SceneCottage()
{
	sceneSettings.enableShadows      = true;
	sceneSettings.enableOmniShadows  = true;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 25.0f, 15.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;

	sceneSettings.directionalLight.base.ambientIntensity = 0.2f;
	sceneSettings.directionalLight.base.diffuseIntensity = 0.8f;
	sceneSettings.directionalLight.direction = glm::vec3(-0.8f, -1.2f, 0.8f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	sceneSettings.pointLights[0].base.enabled = true;
	sceneSettings.pointLights[0].base.color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pointLights[0].position = glm::vec3(0.0f, 6.0f, 0.0f);
	sceneSettings.pointLights[0].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[0].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[0].constant = 0.4f;
	sceneSettings.pointLights[0].linear   = 0.3f;
	sceneSettings.pointLights[0].exponent = 0.2f;

	sceneSettings.pointLights[1].base.enabled = true;
	sceneSettings.pointLights[1].base.color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[1].position = glm::vec3(-5.0f, 8.0f, -5.0f);
	sceneSettings.pointLights[1].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[1].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[1].constant = 0.4f;
	sceneSettings.pointLights[1].linear = 0.3f;
	sceneSettings.pointLights[1].exponent = 0.2f;

	sceneSettings.pointLights[2].base.enabled = true;
	sceneSettings.pointLights[2].base.color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pointLights[2].position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pointLights[2].base.ambientIntensity = 1.0f;
	sceneSettings.pointLights[2].base.diffuseIntensity = 2.0f;
	sceneSettings.pointLights[2].constant = 0.4f;
	sceneSettings.pointLights[2].linear = 0.3f;
	sceneSettings.pointLights[2].exponent = 0.2f;

	sceneSettings.pointLights[3].base.enabled = false;

	sceneSettings.spotLights[0].base.base.enabled = false;
	sceneSettings.spotLights[1].base.base.enabled = false;
	sceneSettings.spotLights[2].base.base.enabled = false;
	sceneSettings.spotLights[3].base.base.enabled = false;

	sceneSettings.shadowMapWidth = 1024;
	sceneSettings.shadowMapHeight = 1024;
	sceneSettings.shadowSpeed = 2.0f;
	sceneSettings.waterHeight = 20.0f;
	sceneSettings.waterWaveSpeed = 0.01f;

	SetSkybox();
	SetupTextures();
	SetupMeshes();
	SetupModels();
}

void SceneCottage::SetSkybox()
{
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");
	m_Skybox = new Skybox(skyboxFaces);
}

void SceneCottage::SetupTextures()
{
	textures.insert(std::make_pair("brick",              TextureLoader::Get()->GetTexture("Textures/brick.png", false, false)));
	textures.insert(std::make_pair("crateDiffuse",       TextureLoader::Get()->GetTexture("Textures/crate.png", false, false)));
	textures.insert(std::make_pair("crateNormal",        TextureLoader::Get()->GetTexture("Textures/crateNormal.png", false, false)));
	textures.insert(std::make_pair("grass",              TextureLoader::Get()->GetTexture("Textures/grass.jpg", false, false)));
	textures.insert(std::make_pair("sponzaFloorDiffuse", TextureLoader::Get()->GetTexture("Textures/sponza_floor_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaFloorNormal",  TextureLoader::Get()->GetTexture("Textures/sponza_floor_a_ddn.tga", false, false)));
	textures.insert(std::make_pair("sponzaWallDiffuse",  TextureLoader::Get()->GetTexture("Textures/sponza_bricks_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaWallNormal",   TextureLoader::Get()->GetTexture("Textures/sponza_bricks_a_ddn.tga", false, false)));
	textures.insert(std::make_pair("sponzaCeilDiffuse",  TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_diff.tga", false, false)));
	textures.insert(std::make_pair("sponzaCeilNormal",   TextureLoader::Get()->GetTexture("Textures/sponza_ceiling_a_ddn.tga", false, false)));
}

void SceneCottage::SetupMeshes()
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
}

void SceneCottage::SetupModels()
{
	Model* cottage = new Model();
	cottage->LoadModel("Models/cottage.obj");
	models.insert(std::make_pair("cottage", cottage));

	Sphere* sphere = new Sphere();
	sphere->Create();
	meshes.insert(std::make_pair("sphere", sphere));
}

void SceneCottage::Update(float timestep, Window* mainWindow)
{
	m_Camera->OnUpdate(timestep);
}

void SceneCottage::UpdateImGui(float timestep, Window* mainWindow)
{
	bool p_open = true;
	ShowExampleAppDockSpace(&p_open, mainWindow);

	ImGui::Begin("Camera");
	{
		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			char buffer[100];
			sprintf(buffer, "Pitch         %.2f", m_Camera->GetPitch());
			ImGui::Text(buffer);
			sprintf(buffer, "Yaw           %.2f", m_Camera->GetYaw());
			ImGui::Text(buffer);
			sprintf(buffer, "FOV           %.2f", glm::degrees(m_Camera->GetPerspectiveVerticalFOV()));
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

	ImGui::Begin("Lights");
	{
		SDirectionalLight directionalLight;
		SPointLight pointLights[4];

		directionalLight.base.enabled = LightManager::directionalLight.GetEnabled();
		directionalLight.direction = LightManager::directionalLight.GetDirection();
		directionalLight.base.color = LightManager::directionalLight.GetColor();
		directionalLight.base.ambientIntensity = LightManager::directionalLight.GetAmbientIntensity();
		directionalLight.base.diffuseIntensity = LightManager::directionalLight.GetDiffuseIntensity();

		pointLights[0].position = sceneSettings.pointLights[0].position;
		float lightRadius = 6.0;
		float lightAngle = timestep * sceneSettings.shadowSpeed;
		pointLights[0].position.x += (float)cos(lightAngle) * lightRadius;
		pointLights[0].position.z += (float)sin(lightAngle) * lightRadius;
		pointLights[0].position.y += (float)cos(lightAngle * 0.5) * lightRadius * 0.5f;
		LightManager::pointLights[0].SetPosition(pointLights[0].position);

		pointLights[0].base.enabled = LightManager::pointLights[0].GetEnabled();
		pointLights[0].position = LightManager::pointLights[0].GetPosition();
		pointLights[0].base.color = LightManager::pointLights[0].GetColor();
		pointLights[0].base.ambientIntensity = LightManager::pointLights[0].GetAmbientIntensity();
		pointLights[0].base.diffuseIntensity = LightManager::pointLights[0].GetDiffuseIntensity();
		pointLights[0].constant = LightManager::pointLights[0].GetConstant();
		pointLights[0].linear = LightManager::pointLights[0].GetLinear();
		pointLights[0].exponent = LightManager::pointLights[0].GetExponent();

		pointLights[1].base.enabled = LightManager::pointLights[1].GetEnabled();
		pointLights[1].position = LightManager::pointLights[1].GetPosition();
		pointLights[1].base.color = LightManager::pointLights[1].GetColor();
		pointLights[1].base.ambientIntensity = LightManager::pointLights[1].GetAmbientIntensity();
		pointLights[1].base.diffuseIntensity = LightManager::pointLights[1].GetDiffuseIntensity();
		pointLights[1].constant = LightManager::pointLights[1].GetConstant();
		pointLights[1].linear = LightManager::pointLights[1].GetLinear();
		pointLights[1].exponent = LightManager::pointLights[1].GetExponent();

		pointLights[2].base.enabled = LightManager::pointLights[2].GetEnabled();
		pointLights[2].position = LightManager::pointLights[2].GetPosition();
		pointLights[2].base.color = LightManager::pointLights[2].GetColor();
		pointLights[2].base.ambientIntensity = LightManager::pointLights[2].GetAmbientIntensity();
		pointLights[2].base.diffuseIntensity = LightManager::pointLights[2].GetDiffuseIntensity();
		pointLights[2].constant = LightManager::pointLights[2].GetConstant();
		pointLights[2].linear = LightManager::pointLights[2].GetLinear();
		pointLights[2].exponent = LightManager::pointLights[2].GetExponent();

		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Checkbox("    DL Enabled",           &directionalLight.base.enabled);
			ImGui::ColorEdit3(  "DL Color",             glm::value_ptr(directionalLight.base.color));
			ImGui::SliderFloat3("DL Direction",         glm::value_ptr(directionalLight.direction), -1.0f, 1.0f);
			ImGui::SliderFloat( "DL Ambient Intensity", &directionalLight.base.ambientIntensity, -10.0f, 10.0f);
			ImGui::SliderFloat( "DL Diffuse Intensity", &directionalLight.base.diffuseIntensity, -10.0f, 10.0f);

			ImGui::Checkbox(    "PL 0 Enabled",           &pointLights[0].base.enabled);
			ImGui::ColorEdit3(  "PL 0 Color",             glm::value_ptr(pointLights[0].base.color));
			ImGui::SliderFloat3("PL 0 Position",          glm::value_ptr(pointLights[0].position), -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 0 Ambient Intensity", &pointLights[0].base.ambientIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 0 Diffuse Intensity", &pointLights[0].base.diffuseIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 0 Constant",          &pointLights[0].constant, -2.0f, 2.0f);
			ImGui::SliderFloat( "PL 0 Linear",            &pointLights[0].linear, -2.0f, 2.0f);
			ImGui::SliderFloat( "PL 0 Exponent",          &pointLights[0].exponent, -2.0f, 2.0f);

			ImGui::Checkbox(    "PL 1 Enabled",           &pointLights[1].base.enabled);
			ImGui::ColorEdit3(  "PL 1 Color",             glm::value_ptr(pointLights[1].base.color));
			ImGui::SliderFloat3("PL 1 Position",          glm::value_ptr(pointLights[1].position), -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 1 Ambient Intensity", &pointLights[1].base.ambientIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 1 Diffuse Intensity", &pointLights[1].base.diffuseIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat( "PL 1 Constant",          &pointLights[1].constant, -2.0f, 2.0f);
			ImGui::SliderFloat( "PL 1 Linear",            &pointLights[1].linear, -2.0f, 2.0f);
			ImGui::SliderFloat( "PL 1 Exponent",          &pointLights[1].exponent, -2.0f, 2.0f);

			ImGui::Checkbox("PL 2 Enabled",              &pointLights[2].base.enabled);
			ImGui::ColorEdit3("PL 2 Color",              glm::value_ptr(pointLights[2].base.color));
			ImGui::SliderFloat3("PL 2 Position",         glm::value_ptr(pointLights[2].position), -20.0f, 20.0f);
			ImGui::SliderFloat("PL 2 Ambient Intensity", &pointLights[2].base.ambientIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat("PL 2 Diffuse Intensity", &pointLights[2].base.diffuseIntensity, -20.0f, 20.0f);
			ImGui::SliderFloat("PL 2 Constant",          &pointLights[2].constant, -2.0f, 2.0f);
			ImGui::SliderFloat("PL 2 Linear",            &pointLights[2].linear, -2.0f, 2.0f);
			ImGui::SliderFloat("PL 2 Exponent",          &pointLights[2].exponent, -2.0f, 2.0f);
		}

		LightManager::directionalLight.SetEnabled(directionalLight.base.enabled);
		LightManager::directionalLight.SetDirection(directionalLight.direction);
		LightManager::directionalLight.SetColor(directionalLight.base.color);
		LightManager::directionalLight.SetAmbientIntensity(directionalLight.base.ambientIntensity);
		LightManager::directionalLight.SetDiffuseIntensity(directionalLight.base.diffuseIntensity);

		LightManager::pointLights[0].SetEnabled(pointLights[0].base.enabled);
		LightManager::pointLights[0].SetPosition(pointLights[0].position);
		LightManager::pointLights[0].SetColor(pointLights[0].base.color);
		LightManager::pointLights[0].SetAmbientIntensity(pointLights[0].base.ambientIntensity);
		LightManager::pointLights[0].SetDiffuseIntensity(pointLights[0].base.diffuseIntensity);
		LightManager::pointLights[0].SetConstant(pointLights[0].constant);
		LightManager::pointLights[0].SetLinear(pointLights[0].linear);
		LightManager::pointLights[0].SetExponent(pointLights[0].exponent);

		LightManager::pointLights[1].SetEnabled(pointLights[1].base.enabled);
		LightManager::pointLights[1].SetPosition(pointLights[1].position);
		LightManager::pointLights[1].SetColor(pointLights[1].base.color);
		LightManager::pointLights[1].SetAmbientIntensity(pointLights[1].base.ambientIntensity);
		LightManager::pointLights[1].SetDiffuseIntensity(pointLights[1].base.diffuseIntensity);
		LightManager::pointLights[1].SetConstant(pointLights[1].constant);
		LightManager::pointLights[1].SetLinear(pointLights[1].linear);
		LightManager::pointLights[1].SetExponent(pointLights[1].exponent);

		LightManager::pointLights[2].SetEnabled(pointLights[2].base.enabled);
		LightManager::pointLights[2].SetPosition(pointLights[2].position);
		LightManager::pointLights[2].SetColor(pointLights[2].base.color);
		LightManager::pointLights[2].SetAmbientIntensity(pointLights[2].base.ambientIntensity);
		LightManager::pointLights[2].SetDiffuseIntensity(pointLights[2].base.diffuseIntensity);
		LightManager::pointLights[2].SetConstant(pointLights[2].constant);
		LightManager::pointLights[2].SetLinear(pointLights[2].linear);
		LightManager::pointLights[2].SetExponent(pointLights[2].exponent);
	}
	ImGui::End();

	ImGui::Begin("Framebuffers");
	{
		if (ImGui::CollapsingHeader("Display Info", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImVec2 imageSize(96.0f, 96.0f);

			ImGui::Text("Shadow Map");
			ImGui::Image((void*)(intptr_t)LightManager::directionalLight.GetShadowMap()->GetTextureID(), imageSize);

			if (ImGui::CollapsingHeader("Omni Shadow Maps"))
			{
				ImGui::Text("Omni Shadow Map 0\n(Point Light 0)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[0].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 1\n(Point Light 1)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[1].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 2\n(Point Light 2)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[2].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 3\n(Point Light 3)");
				ImGui::Image((void*)(intptr_t)LightManager::pointLights[3].GetShadowMap()->GetTextureID(), imageSize);

				ImGui::Text("Omni Shadow Map 4\n(Spot Light 0)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[0].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 5\n(Spot Light 1)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[1].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 6\n(Spot Light 2)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[2].GetShadowMap()->GetTextureID(), imageSize);
				ImGui::Text("Omni Shadow Map 7\n(Spot Light 3)");
				ImGui::Image((void*)(intptr_t)LightManager::spotLights[3].GetShadowMap()->GetTextureID(), imageSize);
			}
		}
	}
	ImGui::End();
}

void SceneCottage::Render(Window* mainWindow, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, int> uniforms)
{
	glm::mat4 sceneOrigin = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];

	// Model matrix
	glm::mat4 model;

	/* Sphere model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 3.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["sphere"]->Render();

	/* Cube Left */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-5.0f, 3.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["brick"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	materials["shiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	/* Cube Right */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(6.0f, 2.0f, 0.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["crateDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["crateNormal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	/* Cube Front */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(8.0f, 1.0f, 3.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(2.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["crateDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["crateNormal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["cube"]->Render();

	/* Cottage */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 20.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["cottage"]->Render(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

	if (passType == "main")
	{
		/* Floor */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)) * sceneOrigin;
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaFloorDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaFloorNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Floor 2nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f)) * sceneOrigin;
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaFloorDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaFloorNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Floor 3nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 20.0f, 0.0f)) * sceneOrigin;
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["grass"]->Bind(textureSlots["diffuse"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Wall Right */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 10.0f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaWallDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaWallNormal"]->Bind(textureSlots["normal"]);
		materials["shiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		// meshList["quadLarge"]->RenderMesh();

		/* Wall Left */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-10.0f, 10.0f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaWallDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaWallNormal"]->Bind(textureSlots["normal"]);
		materials["shiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Wall Back */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -10.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaWallDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaWallNormal"]->Bind(textureSlots["normal"]);
		materials["shiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Ceil */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 9.99f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* Ceil 2nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 19.99f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->Render();

		/* ShadowMap display */
		model = glm::translate(glm::mat4(1.0f), glm::vec3(-9.95f, 5.0f, 5.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaderMain->setInt("albedoMap", textureSlots["shadow"]);
		shaderMain->setInt("normalMap", textureSlots["shadow"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->Render();
	}
}

SceneCottage::~SceneCottage()
{
}
