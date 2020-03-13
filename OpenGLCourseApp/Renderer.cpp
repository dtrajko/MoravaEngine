#include "Renderer.h"

#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>

#include "ShaderMain.h"
#include "ShaderWater.h"
#include "ShaderPBR.h"
#include "ShaderCubemap.h"
#include "ShaderSkyboxJoey.h"

#include "WaterManager.h"



std::map<std::string, Shader*> Renderer::shaders;
std::map<std::string, GLint> Renderer::uniforms;
glm::vec4 Renderer::bgColor = glm::vec4(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);

RadianceHDR* Renderer::m_RadianceHDR;
Cubemap* Renderer::m_EnvironmentCubemap;
Cube* Renderer::m_Cube1x1;

void Renderer::Init()
{
	SetUniforms();
	SetShaders();

	m_RadianceHDR = new RadianceHDR("Textures/HDR/newport_loft.hdr");
	m_RadianceHDR->Load();
	printf("Renderer m_RadianceHDR Width=%d, Height=%d\n", m_RadianceHDR->GetWidth(), m_RadianceHDR->GetHeight());
	m_EnvironmentCubemap = new Cubemap(512, 512);
	m_Cube1x1 = new Cube();
}

void Renderer::SetUniforms()
{
	// common
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("nearPlane", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
	uniforms.insert(std::make_pair("directionalLightTransform", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("lightPosition", 0));

	// main
	uniforms.insert(std::make_pair("eyePosition", 0));

	// water
	uniforms.insert(std::make_pair("reflectionTexture", 0));
	uniforms.insert(std::make_pair("refractionTexture", 0));
	uniforms.insert(std::make_pair("dudvMap", 0));
	uniforms.insert(std::make_pair("depthMap", 0));
	uniforms.insert(std::make_pair("moveFactor", 0));
	uniforms.insert(std::make_pair("cameraPosition", 0));
	uniforms.insert(std::make_pair("lightColor", 0));

	// PBR - physically based rendering
	uniforms.insert(std::make_pair("albedo", 0));
	uniforms.insert(std::make_pair("metallic", 0));
	uniforms.insert(std::make_pair("roughness", 0));
	uniforms.insert(std::make_pair("ao", 0));
	uniforms.insert(std::make_pair("albedoMap", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("metallicMap", 0));
	uniforms.insert(std::make_pair("roughnessMap", 0));
	uniforms.insert(std::make_pair("aoMap", 0));
	uniforms.insert(std::make_pair("camPos", 0));
	uniforms.insert(std::make_pair("ambientIntensity", 0));

	// cubemap shader
	uniforms.insert(std::make_pair("equirectangularMap", 0));

	// skybox Joey shader
	uniforms.insert(std::make_pair("environmentMap", 0));
}

void Renderer::SetShaders()
{
	static const char* vertShader = "Shaders/shader.vert";
	static const char* fragShader = "Shaders/shader.frag";
	ShaderMain* shaderMain = new ShaderMain();
	shaderMain->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("main", shaderMain));
	printf("Renderer: Main shader compiled [programID=%d]\n", shaderMain->GetProgramID());

	static const char* vertShaderDirShadowMap = "Shaders/directional_shadow_map.vert";
	static const char* fragShaderDirShadowMap = "Shaders/directional_shadow_map.frag";
	Shader* shaderDirectionalShadow = new Shader();
	shaderDirectionalShadow->CreateFromFiles(vertShaderDirShadowMap, fragShaderDirShadowMap);
	shaders.insert(std::make_pair("directionalShadow", shaderDirectionalShadow));
	printf("Renderer: Shadow shader compiled [programID=%d]\n", shaderDirectionalShadow->GetProgramID());

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";
	Shader* shaderOmniShadow = new Shader();
	shaderOmniShadow->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	shaders.insert(std::make_pair("omniShadow", shaderOmniShadow));
	printf("Renderer: OmniShadow shader compiled [programID=%d]\n", shaderOmniShadow->GetProgramID());

	static const char* vertWaterShader = "Shaders/water.vert";
	static const char* fragWaterShader = "Shaders/water.frag";
	ShaderWater* shaderWater = new ShaderWater();
	shaderWater->CreateFromFiles(vertWaterShader, fragWaterShader);
	shaders.insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());

	static const char* vertPBRShader = "Shaders/PBR.vert";
	static const char* fragPBRShader = "Shaders/PBR.frag";
	ShaderPBR* shaderPBR = new ShaderPBR();
	shaderPBR->CreateFromFiles(vertPBRShader, fragPBRShader);
	shaders.insert(std::make_pair("pbr", shaderPBR));
	printf("Renderer: PBR shader compiled [programID=%d]\n", shaderPBR->GetProgramID());

	static const char* vertCubemapShader = "Shaders/cubemap.vert";
	static const char* fragCubemapShader = "Shaders/cubemap.frag";
	ShaderCubemap* shaderCubemap = new ShaderCubemap();
	shaderCubemap->CreateFromFiles(vertCubemapShader, fragCubemapShader);
	shaders.insert(std::make_pair("cubemap", shaderCubemap));
	printf("Renderer: Cubemap shader compiled [programID=%d]\n", shaderCubemap->GetProgramID());

	static const char* vertSkyboxJoey = "Shaders/skybox_joey.vert";
	static const char* fragSkyboxJoey = "Shaders/skybox_joey.frag";
	ShaderSkyboxJoey* skyboxJoeyShader = new ShaderSkyboxJoey();
	skyboxJoeyShader->CreateFromFiles(vertSkyboxJoey, fragSkyboxJoey);
	shaders.insert(std::make_pair("skybox_joey", skyboxJoeyShader));
	printf("Renderer: SkyboxJoey shader compiled [programID=%d]\n", skyboxJoeyShader->GetProgramID());
}

void Renderer::RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera, WaterManager* waterManager)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (scene->GetSettings().enableSkybox)
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		float angleRadians = glm::radians((GLfloat)glfwGetTime());
		modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
		scene->GetSkybox()->Draw(modelMatrix, camera->CalculateViewMatrix(), projectionMatrix);
	}

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"] = shaderMain->GetModelLocation();
	uniforms["projection"] = shaderMain->GetProjectionLocation();
	uniforms["view"] = shaderMain->GetViewLocation();
	uniforms["eyePosition"] = shaderMain->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderMain->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderMain->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaderMain->SetClipPlane(glm::vec4(0.0f, -1.0f, 0.0f, -10000));

	shaderMain->Validate();

	glm::vec3 lowerLight = camera->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, camera->GetDirection());

	std::string passType = "main";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	ShaderWater* shaderWater = (ShaderWater*)shaders["water"];
	shaderWater->Bind();
	uniforms["model"] = shaderWater->GetModelLocation();
	uniforms["projection"] = shaderWater->GetProjectionLocation();
	uniforms["view"] = shaderWater->GetViewLocation();
	uniforms["reflectionTexture"] = shaderWater->GetUniformLocationReflectionTexture();
	uniforms["refractionTexture"] = shaderWater->GetUniformLocationRefractionTexture();
	uniforms["dudvMap"] = shaderWater->GetUniformLocationDuDvMap();
	uniforms["normalMap"] = shaderWater->GetUniformLocationNormalMap();
	uniforms["depthMap"] = shaderWater->GetUniformLocationDepthMap();
	uniforms["moveFactor"] = shaderWater->GetUniformLocationMoveFactor();
	uniforms["cameraPosition"] = shaderWater->GetUniformLocationCameraPosition();
	uniforms["lightColor"] = shaderWater->GetUniformLocationLightColor();
	uniforms["lightPosition"] = shaderWater->GetUniformLocationLightPosition();
	uniforms["nearPlane"] = shaderWater->GetUniformLocationNearPlane();
	uniforms["farPlane"] = shaderWater->GetUniformLocationFarPlane();

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);
	scene->GetTextures()["waterDuDv"]->Bind(scene->GetTextureSlots()["DuDv"]);

	shaderWater->SetNearPlane(scene->GetSettings().nearPlane);
	shaderWater->SetFarPlane(scene->GetSettings().farPlane);
	shaderWater->SetWater(scene->GetTextureSlots()["reflection"], scene->GetTextureSlots()["refraction"],
		scene->GetTextureSlots()["DuDv"], scene->GetTextureSlots()["depth"]);
	shaderWater->SetMoveFactor(waterManager->GetWaterMoveFactor());
	shaderWater->SetCameraPosition(camera->GetPosition());
	shaderWater->SetLightColor(LightManager::directionalLight.GetColor());
	shaderWater->SetLightPosition(LightManager::directionalLight.GetPosition());
	shaderWater->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderWater->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderWater->SetDuDvMap(scene->GetTextureSlots()["DuDv"]);
	shaderWater->Validate();

	passType = "main";
	scene->RenderWater(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);
	shaderWater->Unbind();

	ShaderPBR* shaderPBR = static_cast<ShaderPBR*>(shaders["pbr"]);

	shaderPBR->Bind();
	uniforms["model"] = shaderPBR->GetModelLocation();
	uniforms["projection"] = shaderPBR->GetProjectionLocation();
	uniforms["view"] = shaderPBR->GetViewLocation();

	shaderPBR->SetCameraPosition(camera->GetPosition());

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	scene->RenderPBR(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms);

	RenderSimpleSkyboxJoey(camera->CalculateViewMatrix(), projectionMatrix, scene);
}

void Renderer::RenderPassShadow(DirectionalLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager)
{
	if (!scene->GetSettings().enableShadows) return;

	shaders["directionalShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["directionalShadow"]->GetModelLocation();
	shaders["directionalShadow"]->SetDirectionalLightTransform(&light->CalculateLightTransform());

	shaders["directionalShadow"]->Validate();

	std::string passType = "shadow";
	scene->Render(viewMatrix, projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderOmniShadows(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
		Renderer::RenderPassOmniShadow(&LightManager::pointLights[i], viewMatrix, projectionMatrix, scene, waterManager);

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
		Renderer::RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], viewMatrix, projectionMatrix, scene, waterManager);
}

void Renderer::RenderPassOmniShadow(PointLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager)
{
	shaders["omniShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["omniShadow"]->GetModelLocation();
	uniforms["omniLightPos"] = shaders["omniShadow"]->GetUniformLocationLightPosition();
	uniforms["farPlane"] = shaders["omniShadow"]->GetUniformLocationFarPlane();

	glUniform3f(uniforms["omniLightPos"], light->GetPosition().x, light->GetPosition().y, light->GetPosition().z);
	glUniform1f(uniforms["farPlane"], light->GetFarPlane());

	shaders["omniShadow"]->SetLightMatrices(light->CalculateLightTransform());

	shaders["omniShadow"]->Validate();

	std::string passType = "shadow";
	scene->Render(viewMatrix, projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderWaterEffects(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera, float deltaTime)
{
	if (!scene->GetSettings().enableWaterEffects) return;

	glEnable(GL_CLIP_DISTANCE0);
	float waterMoveFactor = waterManager->GetWaterMoveFactor();
	waterMoveFactor += WaterManager::m_WaveSpeed * deltaTime;
	if (waterMoveFactor >= 1.0f)
		waterMoveFactor = waterMoveFactor - 1.0f;
	waterManager->SetWaterMoveFactor(waterMoveFactor);

	float distance = 2.0f * (camera->GetPosition().y - waterManager->GetWaterHeight());
	camera->SetPosition(glm::vec3(camera->GetPosition().x, camera->GetPosition().y - distance, camera->GetPosition().z));
	camera->InvertPitch();

	Renderer::RenderPassWaterReflection(waterManager, projectionMatrix, scene, camera);

	camera->SetPosition(glm::vec3(camera->GetPosition().x, camera->GetPosition().y + distance, camera->GetPosition().z));
	camera->InvertPitch();

	Renderer::RenderPassWaterRefraction(waterManager, projectionMatrix, scene, camera);
}

void Renderer::RenderPassWaterReflection(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera)
{
	glViewport(0, 0, waterManager->GetFramebufferWidth(), waterManager->GetFramebufferHeight());

	waterManager->GetReflectionFramebuffer()->Bind();

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 modelMatrixSkybox = glm::mat4(1.0f);
	float angleRadians = glm::radians((GLfloat)glfwGetTime());
	modelMatrixSkybox = glm::rotate(modelMatrixSkybox, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	scene->GetSkybox()->Draw(modelMatrixSkybox, camera->CalculateViewMatrix(), projectionMatrix);

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"] = shaderMain->GetModelLocation();
	uniforms["projection"] = shaderMain->GetProjectionLocation();
	uniforms["view"] = shaderMain->GetViewLocation();
	uniforms["eyePosition"] = shaderMain->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderMain->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderMain->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaderMain->SetClipPlane(glm::vec4(0.0f, 1.0f, 0.0f, -waterManager->GetWaterHeight())); // reflection clip plane

	shaderMain->Validate();

	std::string passType = "water";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterRefraction(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera)
{
	glViewport(0, 0, waterManager->GetFramebufferWidth(), waterManager->GetFramebufferHeight());

	waterManager->GetRefractionFramebuffer()->Bind();
	waterManager->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
	waterManager->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"] = shaderMain->GetModelLocation();
	uniforms["projection"] = shaderMain->GetProjectionLocation();
	uniforms["view"] = shaderMain->GetViewLocation();
	uniforms["eyePosition"] = shaderMain->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderMain->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderMain->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaderMain->SetClipPlane(glm::vec4(0.0f, -1.0f, 0.0f, waterManager->GetWaterHeight())); // refraction clip plane

	shaderMain->Validate();

	std::string passType = "water";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderEnvironmentCubemap(Window& mainWindow, Scene* scene)
{
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	};

	// convert HDR equirectangular environment map to cubemap equivalent
	ShaderCubemap* shaderCubemap = (ShaderCubemap*)shaders["cubemap"];

	shaderCubemap->Bind();

	uniforms["equirectangularMap"] = shaderCubemap->GetUniformLocationEquirectangularMap();

	shaderCubemap->SetEquirectangularMap(scene->GetTextureSlots()["equirectangularMap"]);
	shaderCubemap->SetProjectionMatrix(&captureProjection);

	// printf("Renderer: bind m_RadianceHDR ID=%d to slot=%d\n", m_RadianceHDR->GetID(), scene->GetTextureSlots()["equirectangularMap"]);
	m_RadianceHDR->Bind(scene->GetTextureSlots()["equirectangularMap"]);

	m_EnvironmentCubemap->GetTextureCubemap()->Bind(0); // scene->GetTextureSlots()["environmentMap"]

	glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
	m_EnvironmentCubemap->GetCaptureFBO()->Bind();
	glBindFramebuffer(GL_FRAMEBUFFER, m_EnvironmentCubemap->GetCaptureFBO()->GetID());

	// printf("m_EnvironmentCubemap GetCaptureFBO.GetID=%d\n", m_EnvironmentCubemap->GetCaptureFBO()->GetID());

	for (unsigned int i = 0; i < 6; ++i)
	{
		shaderCubemap->SetViewMatrix(&captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			m_EnvironmentCubemap->GetTextureCubemap()->GetID(), 0);
		// printf("Renderer m_EnvironmentCubemap GetID=%d\n", m_EnvironmentCubemap->GetTextureCubemap()->GetID());
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_Cube1x1->Render();
	}

	m_EnvironmentCubemap->GetCaptureFBO()->Unbind();
	shaderCubemap->Unbind();

	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());
}

void Renderer::RenderSimpleSkyboxJoey(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene)
{
	glDepthFunc(GL_LEQUAL);

	ShaderSkyboxJoey* shaderSkyboxJoey = static_cast<ShaderSkyboxJoey*>(shaders["skybox_joey"]);

	shaderSkyboxJoey->Bind();

	uniforms["environmentMap"] = shaderSkyboxJoey->GetUniformLocationEnvironmentMap();

	shaderSkyboxJoey->SetEnvironmentMap(scene->GetTextureSlots()["environmentMap"]);
	shaderSkyboxJoey->SetProjectionMatrix(&projectionMatrix);
	shaderSkyboxJoey->SetViewMatrix(&viewMatrix);
	m_EnvironmentCubemap->GetTextureCubemap()->Bind(scene->GetTextureSlots()["environmentMap"]);
	m_Cube1x1->Render();
	shaderSkyboxJoey->Unbind();
}

void Renderer::EnableCulling()
{
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
}

void Renderer::DisableCulling()
{
	glDisable(GL_CULL_FACE);
}

void Renderer::Cleanup()
{
	for (auto& shader : shaders)
		delete shader.second;

	shaders.clear();
	uniforms.clear();
}
