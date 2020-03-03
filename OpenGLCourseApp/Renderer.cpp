#include "Renderer.h"

#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>

#include "WaterManager.h"



std::map<std::string, Shader*> Renderer::shaders;
std::map<std::string, GLint> Renderer::uniforms;
glm::vec4 Renderer::bgColor = glm::vec4(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);

void Renderer::Init()
{
	SetUniforms();
	SetShaders();
}

void Renderer::SetUniforms()
{
	uniforms.insert(std::make_pair("model", 0));
	uniforms.insert(std::make_pair("view", 0));
	uniforms.insert(std::make_pair("projection", 0));
	uniforms.insert(std::make_pair("eyePosition", 0));
	uniforms.insert(std::make_pair("specularIntensity", 0));
	uniforms.insert(std::make_pair("shininess", 0));
	uniforms.insert(std::make_pair("directionalLightTransform", 0));
	uniforms.insert(std::make_pair("omniLightPos", 0));
	uniforms.insert(std::make_pair("farPlane", 0));
	uniforms.insert(std::make_pair("reflectionTexture", 0));
	uniforms.insert(std::make_pair("refractionTexture", 0));
	uniforms.insert(std::make_pair("dudvMap", 0));
	uniforms.insert(std::make_pair("normalMap", 0));
	uniforms.insert(std::make_pair("waterMoveFactor", 0));
	uniforms.insert(std::make_pair("cameraPosition", 0));
	uniforms.insert(std::make_pair("lightColor", 0));
	uniforms.insert(std::make_pair("lightPosition", 0));
}

void Renderer::SetShaders()
{
	static const char* vertShader = "Shaders/shader.vert";
	static const char* fragShader = "Shaders/shader.frag";

	static const char* vertShaderDirShadowMap = "Shaders/directional_shadow_map.vert";
	static const char* fragShaderDirShadowMap = "Shaders/directional_shadow_map.frag";

	static const char* vertShaderOmniShadowMap = "Shaders/omni_shadow_map.vert";
	static const char* geomShaderOmniShadowMap = "Shaders/omni_shadow_map.geom";
	static const char* fragShaderOmniShadowMap = "Shaders/omni_shadow_map.frag";

	static const char* vertWaterShader = "Shaders/water.vert";
	static const char* fragWaterShader = "Shaders/water.frag";

	Shader* mainShader = new Shader();
	mainShader->CreateFromFiles(vertShader, fragShader);
	shaders.insert(std::make_pair("main", mainShader));

	Shader* directionalShadowShader = new Shader();
	directionalShadowShader->CreateFromFiles(vertShaderDirShadowMap, fragShaderDirShadowMap);
	shaders.insert(std::make_pair("directionalShadow", directionalShadowShader));

	Shader* omniShadowShader = new Shader();
	omniShadowShader->CreateFromFiles(vertShaderOmniShadowMap, geomShaderOmniShadowMap, fragShaderOmniShadowMap);
	shaders.insert(std::make_pair("omniShadow", omniShadowShader));

	Shader* waterShader = new Shader();
	waterShader->CreateFromFiles(vertWaterShader, fragWaterShader);
	shaders.insert(std::make_pair("water", waterShader));
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

	shaders["main"]->Bind();

	uniforms["model"] = shaders["main"]->GetModelLocation();
	uniforms["projection"] = shaders["main"]->GetProjectionLocation();
	uniforms["view"] = shaders["main"]->GetViewLocation();
	uniforms["eyePosition"] = shaders["main"]->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaders["main"]->GetUniformLocationSpecularIntensity();
	uniforms["shininess"] = shaders["main"]->GetUniformLocationShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);

	shaders["main"]->SetDirectionalLight(&LightManager::directionalLight);
	shaders["main"]->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaders["main"]->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaders["main"]->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	shaders["main"]->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaders["main"]->SetNormalMap(scene->GetTextureSlots()["normal"]);

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaders["main"]->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaders["main"]->SetClipPlane(glm::vec4(0.0f, -1.0f, 0.0f, -10000));

	shaders["main"]->Validate();

	glm::vec3 lowerLight = camera->getPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, camera->getDirection());

	std::string passType = "main";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shaders["water"]->Bind();
	uniforms["model"] = shaders["water"]->GetModelLocation();
	uniforms["projection"] = shaders["water"]->GetProjectionLocation();
	uniforms["view"] = shaders["water"]->GetViewLocation();
	uniforms["reflectionTexture"] = shaders["water"]->GetUniformLocationReflectionTexture();
	uniforms["refractionTexture"] = shaders["water"]->GetUniformLocationRefractionTexture();
	uniforms["dudvMap"] = shaders["water"]->GetUniformLocationDuDvMap();
	uniforms["normalMap"] = shaders["water"]->GetUniformLocationNormalMap();
	uniforms["waterMoveFactor"] = shaders["water"]->GetUniformLocationWaterMoveFactor();
	uniforms["cameraPosition"] = shaders["water"]->GetUniformLocationCameraPosition();
	uniforms["lightColor"] = shaders["water"]->GetUniformLocationLightColor();
	uniforms["lightPosition"] = shaders["water"]->GetUniformLocationLightPosition();

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	shaders["water"]->SetWater(scene->GetTextureSlots()["reflection"], scene->GetTextureSlots()["refraction"],
		scene->GetTextureSlots()["DuDv"], scene->GetTextureSlots()["depth"]);
	shaders["water"]->SetWaterMoveFactor(waterManager->GetWaterMoveFactor());
	shaders["water"]->SetCameraPosition(camera->getPosition());
	shaders["water"]->SetLightColor(LightManager::directionalLight.GetColor());
	shaders["water"]->SetLightPosition(LightManager::directionalLight.GetPosition());
	shaders["water"]->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaders["water"]->Validate();

	passType = "main";
	scene->RenderWater(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);
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
	uniforms["omniLightPos"] = shaders["omniShadow"]->GetUniformLocationOmniLightPos();
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

	float distance = 2.0f * (camera->getPosition().y - waterManager->GetWaterHeight());
	camera->SetPosition(glm::vec3(camera->getPosition().x, camera->getPosition().y - distance, camera->getPosition().z));
	camera->InvertPitch();

	Renderer::RenderPassWaterReflection(waterManager, projectionMatrix, scene, camera);

	camera->SetPosition(glm::vec3(camera->getPosition().x, camera->getPosition().y + distance, camera->getPosition().z));
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

	shaders["main"]->Bind();

	uniforms["model"] = shaders["main"]->GetModelLocation();
	uniforms["projection"] = shaders["main"]->GetProjectionLocation();
	uniforms["view"] = shaders["main"]->GetViewLocation();
	uniforms["eyePosition"] = shaders["main"]->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaders["main"]->GetUniformLocationSpecularIntensity();
	uniforms["shininess"] = shaders["main"]->GetUniformLocationShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);

	shaders["main"]->SetDirectionalLight(&LightManager::directionalLight);
	shaders["main"]->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaders["main"]->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaders["main"]->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaders["main"]->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaders["main"]->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaders["main"]->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaders["main"]->SetClipPlane(glm::vec4(0.0f, 1.0f, 0.0f, -waterManager->GetWaterHeight())); // reflection clip plane

	shaders["main"]->Validate();

	std::string passType = "water";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterRefraction(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera)
{
	glViewport(0, 0, waterManager->GetFramebufferWidth(), waterManager->GetFramebufferHeight());

	waterManager->GetRefractionFramebuffer()->Bind();

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shaders["main"]->Bind();

	uniforms["model"] = shaders["main"]->GetModelLocation();
	uniforms["projection"] = shaders["main"]->GetProjectionLocation();
	uniforms["view"] = shaders["main"]->GetViewLocation();
	uniforms["eyePosition"] = shaders["main"]->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaders["main"]->GetUniformLocationSpecularIntensity();
	uniforms["shininess"] = shaders["main"]->GetUniformLocationShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->getPosition().x, camera->getPosition().y, camera->getPosition().z);

	shaders["main"]->SetDirectionalLight(&LightManager::directionalLight);
	shaders["main"]->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaders["main"]->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaders["main"]->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaders["main"]->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaders["main"]->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaders["main"]->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaders["main"]->SetClipPlane(glm::vec4(0.0f, -1.0f, 0.0f, waterManager->GetWaterHeight())); // refraction clip plane

	shaders["main"]->Validate();

	std::string passType = "water";
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, passType, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Cleanup()
{
	for (auto& shader : shaders)
		delete shader.second;

	shaders.clear();
	uniforms.clear();
}
