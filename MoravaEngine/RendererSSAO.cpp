#include "RendererSSAO.h"
#include "Log.h"


RendererSSAO::RendererSSAO()
{
	SetShaders();
}

void RendererSSAO::Init(Scene* scene)
{
}

void RendererSSAO::SetShaders()
{
	Shader* shaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
	shaders.insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererSSAO: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());

	Shader* shaderSSAOGeometry = new Shader("Shaders/learnopengl/ssao_geometry.vs", "Shaders/learnopengl/ssao_geometry.fs");
	shaders.insert(std::make_pair("ssao_geometry", shaderSSAOGeometry));
	Log::GetLogger()->info("RendererSSAO: shaderSSAOGeometry compiled [programID={0}]", shaderSSAOGeometry->GetProgramID());
}

void RendererSSAO::Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	Shader* shaderMain = (Shader*)shaders["main"];
	shaderMain->Bind();

	shaderMain->setMat4("model", glm::mat4(1.0f));
	shaderMain->setMat4("view", scene->GetCameraController()->CalculateViewMatrix());
	shaderMain->setMat4("projection", projectionMatrix);
	shaderMain->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMain->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMain->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMain->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMain->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMain->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	// Point Lights
	unsigned int textureUnit = scene->GetTextureSlots()["omniShadow"];
	unsigned int offset = 0;
	shaderMain->setInt("pointLightCount", LightManager::pointLightCount);

	for (int i = 0; i < (int)LightManager::pointLightCount; i++)
	{
		shaderMain->setInt("pointLights[" + std::to_string(i) + "].base.enabled", LightManager::pointLights[i].GetEnabled());
		shaderMain->setVec3("pointLights[" + std::to_string(i) + "].base.color", LightManager::pointLights[i].GetColor());
		shaderMain->setFloat("pointLights[" + std::to_string(i) + "].base.ambientIntensity", LightManager::pointLights[i].GetAmbientIntensity());
		shaderMain->setFloat("pointLights[" + std::to_string(i) + "].base.diffuseIntensity", LightManager::pointLights[i].GetDiffuseIntensity());
		shaderMain->setVec3("pointLights[" + std::to_string(i) + "].position", LightManager::pointLights[i].GetPosition());
		shaderMain->setFloat("pointLights[" + std::to_string(i) + "].constant", LightManager::pointLights[i].GetConstant());
		shaderMain->setFloat("pointLights[" + std::to_string(i) + "].linear", LightManager::pointLights[i].GetLinear());
		shaderMain->setFloat("pointLights[" + std::to_string(i) + "].exponent", LightManager::pointLights[i].GetExponent());

		LightManager::pointLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		shaderMain->setInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->setFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::pointLights[i].GetFarPlane());
	}

	// Spot Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = LightManager::pointLightCount;
	shaderMain->setInt("spotLightCount", LightManager::spotLightCount);

	for (int i = 0; i < (int)LightManager::spotLightCount; i++)
	{
		shaderMain->setInt("spotLights[" + std::to_string(i) + "].base.base.enabled", LightManager::spotLights[i].GetBasePL()->GetEnabled());
		shaderMain->setVec3("spotLights[" + std::to_string(i) + "].base.base.color", LightManager::spotLights[i].GetBasePL()->GetColor());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].base.base.ambientIntensity", LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].base.base.diffuseIntensity", LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
		shaderMain->setVec3("spotLights[" + std::to_string(i) + "].base.position", LightManager::spotLights[i].GetBasePL()->GetPosition());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].base.constant", LightManager::spotLights[i].GetBasePL()->GetConstant());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].base.linear", LightManager::spotLights[i].GetBasePL()->GetLinear());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].base.exponent", LightManager::spotLights[i].GetBasePL()->GetExponent());
		shaderMain->setVec3("spotLights[" + std::to_string(i) + "].direction", LightManager::spotLights[i].GetDirection());
		shaderMain->setFloat("spotLights[" + std::to_string(i) + "].edge", LightManager::spotLights[i].GetEdge());

		LightManager::spotLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		shaderMain->setInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->setFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::spotLights[i].GetFarPlane());
	}

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMain->setInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->setFloat("tilingFactor", 1.0f);
	shaderMain->setVec4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	RenderOmniShadows(mainWindow, scene, projectionMatrix);
	RenderPass(mainWindow, scene, projectionMatrix);

	shaderMain->Unbind();
}

void RendererSSAO::RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererSSAO::RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
}

void RendererSSAO::RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow.GetBufferWidth(), (GLsizei)mainWindow.GetBufferHeight());

	// Clear the window
	glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

RendererSSAO::~RendererSSAO()
{
}
