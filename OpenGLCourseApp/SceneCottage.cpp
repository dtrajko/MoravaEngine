#include "SceneCottage.h"

#include "ImGuiWrapper.h"

#include "ShaderMain.h"
#include "Sphere.h"


SceneCottage::SceneCottage()
{
	sceneSettings.enableShadows      = false;
	sceneSettings.enableOmniShadows  = true;
	sceneSettings.enablePointLights  = true;
	sceneSettings.enableSpotLights   = true;
	sceneSettings.enableWaterEffects = true;
	sceneSettings.enableSkybox       = true;
	sceneSettings.enableNormalMaps   = true;
	sceneSettings.cameraPosition = glm::vec3(0.0f, 25.0f, 15.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.cameraMoveSpeed = 4.0f;
	sceneSettings.ambientIntensity = 0.2f;
	sceneSettings.diffuseIntensity = 2.0f;
	sceneSettings.lightDirection = glm::vec3(-0.8f, -1.2f, 0.8f);
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 1.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 6.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 6.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pLight_1_position = glm::vec3(-5.0f, 8.0f, -5.0f);
	sceneSettings.pLight_1_diffuseIntensity = 6.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.0f, 0.0f, 1.0f);
	sceneSettings.pLight_2_position = glm::vec3(10.0f, 2.0f, 10.0f);
	sceneSettings.pLight_2_diffuseIntensity = 6.0f;
	sceneSettings.shadowMapWidth = 1024;
	sceneSettings.shadowMapHeight = 1024;
	sceneSettings.shadowSpeed = 2.0f;
	sceneSettings.waterHeight = 20.0f;
	sceneSettings.waterWaveSpeed = 0.01f;

	SetSkybox();
	SetTextures();
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
	skybox = new Skybox(skyboxFaces);
}

void SceneCottage::SetTextures()
{
	textures.insert(std::make_pair("brick", new Texture("Textures/brick.png")));
	textures.insert(std::make_pair("crateDiffuse", new Texture("Textures/crate.png")));
	textures.insert(std::make_pair("crateNormal", new Texture("Textures/crateNormal.png")));
	textures.insert(std::make_pair("grass", new Texture("Textures/grass.jpg")));
	textures.insert(std::make_pair("sponzaFloorDiffuse", new Texture("Textures/sponza_floor_a_diff.tga")));
	textures.insert(std::make_pair("sponzaFloorNormal", new Texture("Textures/sponza_floor_a_ddn.tga")));
	textures.insert(std::make_pair("sponzaWallDiffuse", new Texture("Textures/sponza_bricks_a_diff.tga")));
	textures.insert(std::make_pair("sponzaWallNormal", new Texture("Textures/sponza_bricks_a_ddn.tga")));
	textures.insert(std::make_pair("sponzaCeilDiffuse", new Texture("Textures/sponza_ceiling_a_diff.tga")));
	textures.insert(std::make_pair("sponzaCeilNormal", new Texture("Textures/sponza_ceiling_a_ddn.tga")));

	textures["brick"]->LoadTexture();
	textures["crateDiffuse"]->LoadTexture();
	textures["crateNormal"]->LoadTexture();
	textures["grass"]->LoadTexture();
	textures["sponzaFloorDiffuse"]->LoadTexture();
	textures["sponzaFloorNormal"]->LoadTexture();
	textures["sponzaWallDiffuse"]->LoadTexture();
	textures["sponzaWallNormal"]->LoadTexture();
	textures["sponzaCeilDiffuse"]->LoadTexture();
	textures["sponzaCeilNormal"]->LoadTexture();
}

void SceneCottage::SetupModels()
{
	Model* cottage = new Model();
	cottage->LoadModel("Models/cottage.obj");
	models.insert(std::make_pair("cottage", cottage));

	Sphere* sphere = new Sphere();
	sphere->CreateMesh();
	meshes.insert(std::make_pair("sphere", sphere));
}

void SceneCottage::Update(float timestep, LightManager& lightManager, WaterManager* waterManager)
{
	glm::vec3 pLightPos = sceneSettings.pLight_0_position;
	float lightRadius = 6.0;
	float lightAngle = timestep * sceneSettings.shadowSpeed;
	pLightPos.x += (float)cos(lightAngle) * lightRadius;
	pLightPos.z += (float)sin(lightAngle) * lightRadius;
	pLightPos.y += (float)cos(lightAngle * 0.5) * lightRadius * 0.5f;
	lightManager.pointLights[0].SetPosition(pLightPos);

	glm::vec3 PL0_Position  = lightManager.pointLights[0].GetPosition();
	glm::vec3 PL0_Color     = lightManager.pointLights[0].GetColor();
	float PL0_AmbIntensity  = lightManager.pointLights[0].GetAmbientIntensity();
	float PL0_DiffIntensity = lightManager.pointLights[0].GetDiffuseIntensity();
	glm::vec3 PL1_Position  = lightManager.pointLights[1].GetPosition();
	glm::vec3 PL1_Color     = lightManager.pointLights[1].GetColor();
	float PL1_AmbIntensity  = lightManager.pointLights[1].GetAmbientIntensity();
	float PL1_DiffIntensity = lightManager.pointLights[1].GetDiffuseIntensity();

	ImGui::SliderFloat3("PL0 Position",      glm::value_ptr(PL0_Position), -20.0f, 20.0f);
	ImGui::ColorEdit3("PL0 Color",           glm::value_ptr(PL0_Color));
	ImGui::SliderFloat("PL0 Amb Intensity",  &PL0_AmbIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat("PL0 Diff Intensity", &PL0_DiffIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat3("PL1 Position",      glm::value_ptr(PL1_Position), -20.0f, 20.0f);
	ImGui::ColorEdit3("PL1 Color",           glm::value_ptr(PL1_Color));
	ImGui::SliderFloat("PL1 Amb Intensity",  &PL1_AmbIntensity, -20.0f, 20.0f);
	ImGui::SliderFloat("PL1 Diff Intensity", &PL1_DiffIntensity, -20.0f, 20.0f);

	lightManager.pointLights[0].SetPosition(PL0_Position);
	lightManager.pointLights[0].SetColor(PL0_Color);
	lightManager.pointLights[0].SetAmbientIntensity(PL0_AmbIntensity);
	lightManager.pointLights[0].SetDiffuseIntensity(PL0_DiffIntensity);
	lightManager.pointLights[1].SetPosition(PL1_Position);
	lightManager.pointLights[1].SetColor(PL1_Color);
	lightManager.pointLights[1].SetAmbientIntensity(PL1_AmbIntensity);
	lightManager.pointLights[1].SetDiffuseIntensity(PL1_DiffIntensity);
}

void SceneCottage::Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
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
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["normalMapDefault"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	meshes["sphere"]->RenderMesh();

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
	meshes["cube"]->RenderMesh();

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
	meshes["cube"]->RenderMesh();

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
	meshes["cube"]->RenderMesh();

	/* Cottage */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 20.0f, -5.0f)) * sceneOrigin;
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["cottage"]->RenderModel(textureSlots["diffuse"], textureSlots["normal"], sceneSettings.enableNormalMaps);

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
		meshes["quadLarge"]->RenderMesh();

		/* Floor 2nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, 0.0f)) * sceneOrigin;
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaFloorDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaFloorNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->RenderMesh();

		/* Floor 3nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 20.0f, 0.0f)) * sceneOrigin;
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["grass"]->Bind(textureSlots["diffuse"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->RenderMesh();

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
		meshes["quadLarge"]->RenderMesh();

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
		meshes["quadLarge"]->RenderMesh();

		/* Ceil */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 9.99f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->RenderMesh();

		/* Ceil 2nd */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 19.99f, 0.0f)) * sceneOrigin;
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
		textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quadLarge"]->RenderMesh();

		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-9.95f, 5.0f, 5.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(2.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaderMain->SetTexture(textureSlots["shadow"]);
		shaderMain->SetNormalMap(textureSlots["shadow"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->RenderMesh();
	}
}

SceneCottage::~SceneCottage()
{
}
