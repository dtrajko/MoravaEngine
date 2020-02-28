#include "Renderer.h"

#include "GL/glew.h"
#include "glm/gtc/matrix_transform.hpp"
#include <GLFW/glfw3.h>



std::map<std::string, Shader*> Renderer::shaders;
std::map<std::string, GLint> Renderer::uniforms;

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
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(135.0f / 255.0f, 206.0f / 255.0f, 235.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	float angleRadians = glm::radians((GLfloat)glfwGetTime());
	modelMatrix = glm::rotate(modelMatrix, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));

	scene->GetSkybox()->Draw(modelMatrix, camera->CalculateViewMatrix(), projectionMatrix);

	shaders["main"]->Bind();

	uniforms["model"] = shaders["main"]->GetModelLocation();
	uniforms["projection"] = shaders["main"]->GetProjectionLocation();
	uniforms["view"] = shaders["main"]->GetViewLocation();
	uniforms["eyePosition"] = shaders["main"]->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaders["main"]->GetUniformLocationSpecularIntensity();
	uniforms["shininess"] = shaders["main"]->GetUniformLocationShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(camera->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], camera->getCameraPosition().x, camera->getCameraPosition().y, camera->getCameraPosition().z);

	shaders["main"]->SetDirectionalLight(&LightManager::directionalLight);
	shaders["main"]->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaders["main"]->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaders["main"]->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaders["main"]->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaders["main"]->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaders["main"]->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	// Water shader sampler2D uniforms
	shaders["water"]->SetReflectionTexture(scene->GetTextureSlots()["reflectionTexture"]);
	shaders["water"]->SetRefractionTexture(scene->GetTextureSlots()["refractionTexture"]);
	shaders["water"]->SetDuDvMap(scene->GetTextureSlots()["dudvMap"]);
	shaders["water"]->SetNormalMap(scene->GetTextureSlots()["normalMap"]);
	shaders["water"]->SetDepthMap(scene->GetTextureSlots()["depthMap"]);

	glm::vec3 lowerLight = camera->getCameraPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, camera->getCameraDirection());

	shaders["main"]->Validate();

	bool mainPass = true;
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, mainPass, shaders, uniforms, waterManager);
}

void Renderer::RenderPassShadow(DirectionalLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager)
{
	shaders["directionalShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["directionalShadow"]->GetModelLocation();
	shaders["directionalShadow"]->SetDirectionalLightTransform(&light->CalculateLightTransform());

	shaders["directionalShadow"]->Validate();

	bool mainPass = false;
	scene->Render(viewMatrix, projectionMatrix, mainPass, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

	bool mainPass = false;
	scene->Render(viewMatrix, projectionMatrix, mainPass, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterReflection(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera)
{
	glViewport(0, 0, waterManager->GetFramebufferWidth(), waterManager->GetFramebufferHeight());

	shaders["water"]->Bind();

	waterManager->GetReflectionFramebuffer()->Bind(scene->GetTextureSlots()["reflectionTexture"], scene->GetTextureSlots()["depthMap"]); // reflectionTexture, depthMap

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["water"]->GetModelLocation();
	uniforms["eyePosition"] = shaders["water"]->GetUniformLocationEyePosition();

	glUniform3f(uniforms["eyePosition"], 1.0f, 0.0f, 1.0f);

	shaders["water"]->Validate();

	bool mainPass = false;
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, mainPass, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterRefraction(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera)
{
	glViewport(0, 0, waterManager->GetFramebufferWidth(), waterManager->GetFramebufferHeight());

	shaders["water"]->Bind();
	waterManager->GetRefractionFramebuffer()->Bind(scene->GetTextureSlots()["refractionTexture"], scene->GetTextureSlots()["depthMap"]); // refractionTexture, depthMap

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["water"]->GetModelLocation();
	uniforms["eyePosition"] = shaders["water"]->GetUniformLocationEyePosition();

	glUniform3f(uniforms["eyePosition"], 1.0f, 1.0f, 0.0f);

	shaders["water"]->Validate();

	bool mainPass = false;
	scene->Render(camera->CalculateViewMatrix(), projectionMatrix, mainPass, shaders, uniforms, waterManager);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Cleanup()
{
	for (auto& shader : shaders)
		delete shader.second;

	shaders.clear();
	uniforms.clear();
}
