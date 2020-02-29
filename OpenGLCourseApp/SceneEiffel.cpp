#include "SceneEiffel.h"



SceneEiffel::SceneEiffel()
{
	sceneSettings.cameraPosition = glm::vec3(0.0f, 6.0f, 20.0f);
	sceneSettings.lightDirection = glm::vec3(3.0f, -9.0f, -3.0f);
	sceneSettings.cameraStartYaw = -90.0f;
	sceneSettings.ambientIntensity = 0.2f;
	sceneSettings.diffuseIntensity = 0.8f;
	sceneSettings.shadowMapWidth = 2048;
	sceneSettings.shadowMapHeight = 2048;
	sceneSettings.shadowSpeed = 0.4f;
	sceneSettings.pLight_0_color = glm::vec3(1.0f, 0.0f, 1.0f);
	sceneSettings.pLight_0_position = glm::vec3(0.0f, 20.0f, 0.0f);
	sceneSettings.pLight_0_diffuseIntensity = 6.0f;
	sceneSettings.pLight_1_color = glm::vec3(1.0f, 0.0f, 0.0f);
	sceneSettings.pLight_1_position = glm::vec3(-2.0f, 9.6f, 0.0f);
	sceneSettings.pLight_1_diffuseIntensity = 6.0f;
	sceneSettings.pLight_2_color = glm::vec3(0.8f, 0.8f, 0.5f);
	sceneSettings.pLight_2_position = glm::vec3(-2.0f, 4.0f, 0.0f);
	sceneSettings.pLight_2_diffuseIntensity = 6.0f;
	sceneSettings.lightProjectionMatrix = glm::ortho(-16.0f, 16.0f, -16.0f, 16.0f, 0.1f, 32.0f);

	SetSkybox();
	SetTextures();
	SetupMeshes();
	SetupModels();
}

void SceneEiffel::SetSkybox()
{
	skyboxFaces.push_back("Textures/skybox_3/right.png");
	skyboxFaces.push_back("Textures/skybox_3/left.png");
	skyboxFaces.push_back("Textures/skybox_3/top.png");
	skyboxFaces.push_back("Textures/skybox_3/bottom.png");
	skyboxFaces.push_back("Textures/skybox_3/back.png");
	skyboxFaces.push_back("Textures/skybox_3/front.png");
	skybox = new Skybox(skyboxFaces);
}

void SceneEiffel::SetTextures()
{
	textures.insert(std::make_pair("sponzaCeilDiffuse", new Texture("Textures/sponza_ceiling_a_diff.tga")));
	textures.insert(std::make_pair("sponzaCeilNormal", new Texture("Textures/sponza_ceiling_a_ddn.tga")));
	textures.insert(std::make_pair("water", new Texture("Textures/water.png")));
	textures.insert(std::make_pair("normalMapDefault", new Texture("Textures/normal_map_default.png")));
	textures.insert(std::make_pair("waterDuDv", new Texture("Textures/water/waterDuDv.png")));
	textures.insert(std::make_pair("waterNormal", new Texture("Textures/water/waterNormal.png")));

	textures["sponzaCeilDiffuse"]->LoadTexture();
	textures["sponzaCeilNormal"]->LoadTexture();
	textures["water"]->LoadTexture();
	textures["normalMapDefault"]->LoadTexture();
	textures["waterDuDv"]->LoadTexture();
	textures["waterNormal"]->LoadTexture();
}

void SceneEiffel::SetupMeshes()
{
}

void SceneEiffel::SetupModels()
{
	Model* eiffel = new Model();
	eiffel->LoadModel("Models/Eiffel_Tower.obj");
	models.insert(std::make_pair("eiffel", eiffel));

	Model* watchtower = new Model();
	watchtower->LoadModel("Models/wooden_watch_tower.obj");
	models.insert(std::make_pair("watchtower", watchtower));
}

void SceneEiffel::Update(float timestep, LightManager* lightManager)
{
	// Shadow rotation
	glm::vec3 lightDirection = sceneSettings.lightDirection;
	float lightRadius = abs(lightDirection.x);
	float lightAngle = timestep * sceneSettings.shadowSpeed;
	lightDirection.x = (float)cos(lightAngle) * lightRadius;
	lightDirection.z = (float)sin(lightAngle) * lightRadius;
	lightManager->directionalLight.SetDirection(lightDirection);
}

void SceneEiffel::Render(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, std::string passType,
	std::map<std::string, Shader*> shaders, std::map<std::string, GLint> uniforms, WaterManager* waterManager)
{
	glm::mat4 model;

	/* Floor */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(3.0f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	textures["sponzaCeilDiffuse"]->Bind(textureSlots["diffuse"]);
	textures["sponzaCeilNormal"]->Bind(textureSlots["normal"]);
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	if (passType != "shadow")
		meshes["quadLarge"]->RenderMesh();

	/* Eiffel model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(-2.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.0003f, 0.0003f, 0.0003f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["eiffel"]->RenderModel(textureSlots["diffuse"], textureSlots["normal"]);

	/* Watchtower model */
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(4.0f, -0.35f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(0.5f));
	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
	materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
	models["watchtower"]->RenderModel(textureSlots["diffuse"], textureSlots["normal"]);

	if (passType == "main")
	{
		/* Water Tile */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.5f, 15.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(10.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		waterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->RenderMesh();

		/* Water reflection framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		waterManager->GetReflectionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		textures["normalMapDefault"]->Bind();
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->RenderMesh();

		/* Water refraction framebuffer */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		waterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(textureSlots["diffuse"]);
		waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(textureSlots["normal"]);
		materials["superShiny"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->RenderMesh();

		/* ShadowMap display */
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(-12.0f, 10.0f, -20.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f));
		glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(model));
		shaders["main"]->SetTexture(textureSlots["shadow"]);
		shaders["main"]->SetNormalMap(textureSlots["shadow"]);
		materials["dull"]->UseMaterial(uniforms["specularIntensity"], uniforms["shininess"]);
		meshes["quad"]->RenderMesh();
	}
}

SceneEiffel::~SceneEiffel()
{
}
