#include "Renderer.h"

#include "ShaderMain.h"
#include "ShaderWater.h"
#include "ShaderPBR.h"

#include "WaterManager.h"


Renderer::Renderer()
{
}

void Renderer::Init()
{
	SetUniforms();
	SetShaders();
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
}

void Renderer::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
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
		scene->GetSkybox()->Draw(modelMatrix, scene->GetCamera()->CalculateViewMatrix(), projectionMatrix);
	}

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"] = shaderMain->GetModelLocation();
	uniforms["projection"] = shaderMain->GetProjectionLocation();
	uniforms["view"] = shaderMain->GetViewLocation();
	uniforms["eyePosition"] = shaderMain->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);

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

	glm::vec3 lowerLight = scene->GetCamera()->GetPosition();
	lowerLight.y -= 0.2f;
	LightManager::spotLights[2].SetFlash(lowerLight, scene->GetCamera()->GetDirection());

	std::string passType = "main";

	EnableCulling();
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	shaderMain->Unbind();

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
	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);
	scene->GetTextures()["waterDuDv"]->Bind(scene->GetTextureSlots()["DuDv"]);

	shaderWater->SetNearPlane(scene->GetSettings().nearPlane);
	shaderWater->SetFarPlane(scene->GetSettings().farPlane);
	shaderWater->SetWater(scene->GetTextureSlots()["reflection"], scene->GetTextureSlots()["refraction"],
		scene->GetTextureSlots()["DuDv"], scene->GetTextureSlots()["depth"]);
	shaderWater->SetMoveFactor(scene->GetWaterManager()->GetWaterMoveFactor());
	shaderWater->SetCameraPosition(scene->GetCamera()->GetPosition());
	shaderWater->SetLightColor(LightManager::directionalLight.GetColor());
	shaderWater->SetLightPosition(LightManager::directionalLight.GetPosition());
	shaderWater->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderWater->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderWater->SetDuDvMap(scene->GetTextureSlots()["DuDv"]);
	shaderWater->Validate();

	EnableCulling();
	passType = "main";
	scene->RenderWater(projectionMatrix, passType, shaders, uniforms);
	shaderWater->Unbind();

	ShaderPBR* shaderPBR = static_cast<ShaderPBR*>(shaders["pbr"]);

	shaderPBR->Bind();
	uniforms["model"] = shaderPBR->GetModelLocation();
	uniforms["projection"] = shaderPBR->GetProjectionLocation();
	uniforms["view"] = shaderPBR->GetViewLocation();

	shaderPBR->SetCameraPosition(scene->GetCamera()->GetPosition());

	glUniformMatrix4fv(uniforms["model"], 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	DisableCulling();
	scene->RenderPBR(projectionMatrix, passType, shaders, uniforms);
}

void Renderer::RenderPassShadow(Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableShadows) return;

	shaders["directionalShadow"]->Bind();

	DirectionalLight* light = &scene->GetLightManager()->directionalLight;
	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	uniforms["model"] = shaders["directionalShadow"]->GetModelLocation();
	shaders["directionalShadow"]->SetDirectionalLightTransform(&light->CalculateLightTransform());

	shaders["directionalShadow"]->Validate();

	EnableCulling();
	std::string passType = "shadow";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderPassShadow(scene, projectionMatrix);
	RenderOmniShadows(scene, projectionMatrix);
	RenderWaterEffects(deltaTime, scene, projectionMatrix);
	RenderPass(mainWindow, scene, projectionMatrix);
}

void Renderer::RenderOmniShadows(Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
		Renderer::RenderPassOmniShadow(&LightManager::pointLights[i], scene, projectionMatrix);

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
		Renderer::RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], scene, projectionMatrix);
}

void Renderer::RenderPassOmniShadow(PointLight* light, Scene* scene, glm::mat4 projectionMatrix)
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

	EnableCulling();
	std::string passType = "shadow";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderWaterEffects(float deltaTime, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableWaterEffects) return;

	glEnable(GL_CLIP_DISTANCE0);
	float waterMoveFactor = scene->GetWaterManager()->GetWaterMoveFactor();
	waterMoveFactor += WaterManager::m_WaveSpeed * deltaTime;
	if (waterMoveFactor >= 1.0f)
		waterMoveFactor = waterMoveFactor - 1.0f;
	scene->GetWaterManager()->SetWaterMoveFactor(waterMoveFactor);

	float distance = 2.0f * (scene->GetCamera()->GetPosition().y - scene->GetWaterManager()->GetWaterHeight());
	scene->GetCamera()->SetPosition(glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y - distance, scene->GetCamera()->GetPosition().z));
	scene->GetCamera()->InvertPitch();

	Renderer::RenderPassWaterReflection(scene, projectionMatrix);

	scene->GetCamera()->SetPosition(glm::vec3(scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y + distance, scene->GetCamera()->GetPosition().z));
	scene->GetCamera()->InvertPitch();

	Renderer::RenderPassWaterRefraction(scene, projectionMatrix);
}

void Renderer::RenderPassWaterReflection(Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetReflectionFramebuffer()->Bind();

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 modelMatrixSkybox = glm::mat4(1.0f);
	float angleRadians = glm::radians((GLfloat)glfwGetTime());
	modelMatrixSkybox = glm::rotate(modelMatrixSkybox, angleRadians, glm::vec3(0.0f, 1.0f, 0.0f));
	scene->GetSkybox()->Draw(modelMatrixSkybox, scene->GetCamera()->CalculateViewMatrix(), projectionMatrix);

	ShaderMain* shaderMain = (ShaderMain*)shaders["main"];
	shaderMain->Bind();

	uniforms["model"] = shaderMain->GetModelLocation();
	uniforms["projection"] = shaderMain->GetProjectionLocation();
	uniforms["view"] = shaderMain->GetViewLocation();
	uniforms["eyePosition"] = shaderMain->GetUniformLocationEyePosition();
	uniforms["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	uniforms["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderMain->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderMain->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaderMain->SetClipPlane(glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetWaterManager()->GetWaterHeight())); // reflection clip plane

	shaderMain->Validate();

	EnableCulling();
	std::string passType ="water";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::RenderPassWaterRefraction(Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetRefractionFramebuffer()->Bind();
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

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

	glUniformMatrix4fv(uniforms["view"], 1, GL_FALSE, glm::value_ptr(scene->GetCamera()->CalculateViewMatrix()));
	glUniformMatrix4fv(uniforms["projection"], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	glUniform3f(uniforms["eyePosition"], scene->GetCamera()->GetPosition().x, scene->GetCamera()->GetPosition().y, scene->GetCamera()->GetPosition().z);

	shaderMain->SetDirectionalLight(&LightManager::directionalLight);
	shaderMain->SetPointLights(LightManager::pointLights, LightManager::pointLightCount, scene->GetTextureSlots()["omniShadow"], 0);
	shaderMain->SetSpotLights(LightManager::spotLights, LightManager::spotLightCount, scene->GetTextureSlots()["omniShadow"], LightManager::pointLightCount);
	shaderMain->SetDirectionalLightTransform(&LightManager::directionalLight.CalculateLightTransform());

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetTexture(scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetNormalMap(scene->GetTextureSlots()["normal"]);
	shaderMain->SetDepthMap(scene->GetTextureSlots()["depth"]);
	shaderMain->SetDirectionalShadowMap(scene->GetTextureSlots()["shadow"]);

	shaderMain->SetClipPlane(glm::vec4(0.0f, -1.0f, 0.0f, scene->GetWaterManager()->GetWaterHeight())); // refraction clip plane

	shaderMain->Validate();

	std::string passType = "water";
	scene->Render(projectionMatrix, passType, shaders, uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Renderer::~Renderer()
{
}
