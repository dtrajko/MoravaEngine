#include "Renderer/Renderer.h"

#include "Core/Application.h"
#include "Shader/ShaderMain.h"
#include "Shader/ShaderPBR.h"
#include "Water/WaterManager.h"


Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Init(Scene* scene)
{
	SetUniforms();
	SetShaders();
}

void Renderer::SetUniforms()
{
	// common
	RendererBasic::GetUniforms().insert(std::make_pair("model", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("view", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("projection", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("nearPlane", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("farPlane", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("dirLightTransform", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("lightPosition", 0));

	// main
	RendererBasic::GetUniforms().insert(std::make_pair("eyePosition", 0));

	// water
	RendererBasic::GetUniforms().insert(std::make_pair("reflectionTexture", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("refractionTexture", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("dudvMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("depthMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("moveFactor", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("cameraPosition", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("lightColor", 0));

	// PBR - physically based rendering
	RendererBasic::GetUniforms().insert(std::make_pair("albedo", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("metallic", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("roughness", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("ao", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("albedoMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("normalMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("metallicMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("roughnessMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("aoMap", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("camPos", 0));
	RendererBasic::GetUniforms().insert(std::make_pair("ambientIntensity", 0));

	// cubemap shader
	RendererBasic::GetUniforms().insert(std::make_pair("equirectangularMap", 0));

	// skybox Joey shader
	RendererBasic::GetUniforms().insert(std::make_pair("environmentMap", 0));
}

void Renderer::SetShaders()
{
	H2M::RefH2M<ShaderMain> shaderMain = H2M::RefH2M<ShaderMain>::Create("Shaders/shader.vert", "Shaders/shader.frag");
	RendererBasic::GetShaders().insert(std::make_pair("main", shaderMain));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderMain->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderDirectionalShadow = MoravaShader::Create("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("directionalShadow", shaderDirectionalShadow));
	printf("Renderer: Shadow shader compiled [programID=%d]\n", shaderDirectionalShadow->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderOmniShadow = MoravaShader::Create("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("omniShadow", shaderOmniShadow));
	printf("Renderer: OmniShadow shader compiled [programID=%d]\n", shaderOmniShadow->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderWater = MoravaShader::Create("Shaders/waterOriginal.vert", "Shaders/waterOriginal.frag");
	RendererBasic::GetShaders().insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderPBR = MoravaShader::Create("Shaders/PBR.vert", "Shaders/PBR.frag");
	RendererBasic::GetShaders().insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());
}

void Renderer::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableShadows) return;

	RendererBasic::GetShaders()["directionalShadow"]->Bind();

	DirectionalLight* light = &LightManager::directionalLight;
	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	// RendererBasic::GetShaders()["directionalShadow"]->SetMat4("model", glm::mat4(1.0f));
	RendererBasic::GetUniforms()["model"] = RendererBasic::GetShaders()["directionalShadow"]->GetUniformLocation("model");
	RendererBasic::GetShaders()["directionalShadow"]->SetMat4("u_DirLightTransform", light->CalculateLightTransform());
	RendererBasic::GetShaders()["directionalShadow"]->SetBool("u_Animated", false);
	RendererBasic::GetShaders()["directionalShadow"]->Validate();

	DisableCulling();
	std::string passType = "shadow";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
	{
		if (LightManager::pointLights[i].GetEnabled())
		{
			Renderer::RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);
		}
	}

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
	{
		if (LightManager::spotLights[i].GetBasePL()->GetEnabled())
		{
			Renderer::RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
		}
	}
}

void Renderer::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	RendererBasic::GetShaders()["omniShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	RendererBasic::GetUniforms()["model"]        = RendererBasic::GetShaders()["omniShadow"]->GetUniformLocation("model");
	RendererBasic::GetUniforms()["omniLightPos"] = RendererBasic::GetShaders()["omniShadow"]->GetUniformLocation("omniLightPos");
	RendererBasic::GetUniforms()["farPlane"]     = RendererBasic::GetShaders()["omniShadow"]->GetUniformLocation("farPlane");

	RendererBasic::GetShaders()["omniShadow"]->SetFloat3("lightPosition", light->GetPosition());
	RendererBasic::GetShaders()["omniShadow"]->SetFloat("farPlane", light->GetFarPlane());
	RendererBasic::GetShaders()["omniShadow"]->SetLightMatrices(light->CalculateLightTransform());
	std::vector<glm::mat4> lightMatrices = light->CalculateLightTransform();
	for (unsigned int i = 0; i < lightMatrices.size(); i++) {
		RendererBasic::GetShaders()["omniShadow"]->SetMat4("lightMatrices[" + std::to_string(i) + "]", lightMatrices[i]);
	}
	RendererBasic::GetShaders()["omniShadow"]->SetBool("u_Animated", false);
	RendererBasic::GetShaders()["omniShadow"]->Validate();

	EnableCulling();
	std::string passType = "shadow";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderWaterEffects(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableWaterEffects) return;

	glEnable(GL_CLIP_DISTANCE0);

	float waterMoveFactor = scene->GetWaterManager()->GetWaterMoveFactor();
	waterMoveFactor += WaterManager::m_WaveSpeed * deltaTime * 0.2f;
	if (waterMoveFactor >= 1.0f)
	{
		waterMoveFactor = waterMoveFactor - 1.0f;
	}
	scene->GetWaterManager()->SetWaterMoveFactor(waterMoveFactor);

	float distance = 2.0f * (scene->GetCamera()->GetPosition().y - scene->GetWaterManager()->GetWaterHeight());
	glm::vec3 cameraPosition = scene->GetCamera()->GetPosition();
	glm::vec3 cameraPositionInverse = scene->GetCamera()->GetPosition();
	cameraPositionInverse.y -= distance;
	scene->GetCamera()->SetPosition(cameraPositionInverse);
	scene->GetCameraController()->InvertPitch();
	// scene->GetCameraController()->InvertRoll();

	RenderPassWaterReflection(mainWindow, scene, projectionMatrix);

	scene->GetCamera()->SetPosition(cameraPosition);
	scene->GetCameraController()->InvertPitch();
	// scene->GetCameraController()->InvertRoll();

	RenderPassWaterRefraction(mainWindow, scene, projectionMatrix);
}

void Renderer::RenderPassWaterReflection(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetReflectionFramebuffer()->Bind();

	// Clear the window
	RendererBasic::Clear();

	glm::mat4 modelMatrixSkybox = glm::mat4(1.0f);
	float angleRadians = glm::radians((GLfloat)glfwGetTime());
	modelMatrixSkybox = glm::rotate(modelMatrixSkybox, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	scene->GetSkybox()->Draw(modelMatrixSkybox, scene->GetCamera()->GetViewMatrix(), projectionMatrix);

	H2M::RefH2M<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"]       = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"]  = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"]        = shaderMain->GetUniformLocation("view");
	RendererBasic::GetUniforms()["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	RendererBasic::GetUniforms()["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	RendererBasic::GetUniforms()["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(RendererBasic::GetUniforms()["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->GetViewMatrix()));
	glUniformMatrix4fv(RendererBasic::GetUniforms()["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(RendererBasic::GetUniforms()["eyePosition"], scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);

	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	EnableCulling();
	std::string passType = "water";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterRefraction(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetRefractionFramebuffer()->Bind();
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

	// Clear the window
	RendererBasic::Clear();

	H2M::RefH2M<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"]       = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"]  = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"]        = shaderMain->GetUniformLocation("view");
	RendererBasic::GetUniforms()["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	RendererBasic::GetUniforms()["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	RendererBasic::GetUniforms()["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	std::string passType = "water";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear();

	if (scene->GetSettings().enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->GetViewMatrix(), projectionMatrix);
	}

	/**** BEGIN shaderMain ****/

	H2M::RefH2M<ShaderMain> shaderMain = (H2M::RefH2M<ShaderMain>)RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"] = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"] = shaderMain->GetUniformLocation("view");
	RendererBasic::GetUniforms()["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	RendererBasic::GetUniforms()["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	RendererBasic::GetUniforms()["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	RendererBasic::GetUniforms()["albedoMap"] = shaderMain->GetUniformLocation("albedoMap");
	RendererBasic::GetUniforms()["normalMap"] = shaderMain->GetUniformLocation("normalMap");
	RendererBasic::GetUniforms()["shadowMap"] = shaderMain->GetUniformLocation("shadowMap");
	RendererBasic::GetUniforms()["clipPlane"] = shaderMain->GetUniformLocation("clipPlane");
	RendererBasic::GetUniforms()["tilingFactor"] = shaderMain->GetUniformLocation("tilingFactor");
	RendererBasic::GetUniforms()["tintColor"] = shaderMain->GetUniformLocation("tintColor");
	RendererBasic::GetUniforms()["dirLightTransform"] = shaderMain->GetUniformLocation("dirLightTransform");

	shaderMain->SetMat4("model", glm::mat4(1.0f));
	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	// clip plane for rendering to screen
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, 10000.0f));
	shaderMain->SetFloat("tilingFactor", 1.0f);
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	glm::vec3 lowerLight = scene->GetCamera()->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, scene->GetCamera()->GetDirection());

	std::string passType = "main";

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	shaderMain->Unbind();

	/**** END shaderMain ****/

	EnableTransparency();

	H2M::RefH2M<MoravaShader> shaderWater = RendererBasic::GetShaders()["water"];
	shaderWater->Bind();
	RendererBasic::GetUniforms()["model"] = shaderWater->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderWater->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"] = shaderWater->GetUniformLocation("view");
	RendererBasic::GetUniforms()["reflectionTexture"] = shaderWater->GetUniformLocation("reflectionTexture");
	RendererBasic::GetUniforms()["refractionTexture"] = shaderWater->GetUniformLocation("refractionTexture");
	RendererBasic::GetUniforms()["dudvMap"] = shaderWater->GetUniformLocation("dudvMap");
	RendererBasic::GetUniforms()["normalMap"] = shaderWater->GetUniformLocation("normalMap");
	RendererBasic::GetUniforms()["depthMap"] = shaderWater->GetUniformLocation("depthMap");
	RendererBasic::GetUniforms()["moveFactor"] = shaderWater->GetUniformLocation("moveFactor");
	RendererBasic::GetUniforms()["cameraPosition"] = shaderWater->GetUniformLocation("cameraPosition");
	RendererBasic::GetUniforms()["lightColor"] = shaderWater->GetUniformLocation("lightColor");
	RendererBasic::GetUniforms()["lightPosition"] = shaderWater->GetUniformLocation("lightPosition");
	RendererBasic::GetUniforms()["nearPlane"] = shaderWater->GetUniformLocation("nearPlane");
	RendererBasic::GetUniforms()["farPlane"] = shaderWater->GetUniformLocation("farPlane");

	shaderWater->SetMat4("model", glm::mat4(1.0f));
	shaderWater->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderWater->SetMat4("projection", projectionMatrix);

	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);
	scene->GetTextures()["waterDuDv"]->Bind(scene->GetTextureSlots()["DuDv"]);
	// shaderWater->SetFloat("nearPlane", scene->GetSettings().nearPlane);
	// shaderWater->SetFloat("farPlane", scene->GetSettings().farPlane);
	shaderWater->SetInt("reflectionTexture", scene->GetTextureSlots()["reflection"]);
	shaderWater->SetInt("refractionTexture", scene->GetTextureSlots()["refraction"]);
	shaderWater->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderWater->SetInt("depthMap", scene->GetTextureSlots()["depth"]);
	shaderWater->SetInt("dudvMap", scene->GetTextureSlots()["DuDv"]);
	shaderWater->SetFloat("moveFactor", scene->GetWaterManager()->GetWaterMoveFactor());
	shaderWater->SetFloat3("cameraPosition", scene->GetCamera()->GetPosition());
	shaderWater->SetFloat3("lightColor", LightManager::directionalLight.GetColor());
	shaderWater->SetFloat3("lightPosition", LightManager::directionalLight.GetPosition());
	shaderWater->Validate();

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();

	passType = "main";
	scene->RenderWater(projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());
	shaderWater->Unbind();
}

void Renderer::BeginFrame()
{
}

void Renderer::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::UpdateProjectionMatrix(&projectionMatrix, scene);

	RenderPassShadow(mainWindow, scene, projectionMatrix);
	RenderOmniShadows(mainWindow, scene, projectionMatrix);
	RenderWaterEffects(deltaTime, mainWindow, scene, projectionMatrix);
	RenderPassMain(mainWindow, scene, projectionMatrix);
}
