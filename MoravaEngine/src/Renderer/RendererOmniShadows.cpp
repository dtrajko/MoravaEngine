#include "Renderer/RendererOmniShadows.h"


RendererOmniShadows::RendererOmniShadows()
{
	SetShaders();
}

RendererOmniShadows::~RendererOmniShadows()
{
}

void RendererOmniShadows::Init(Scene* scene)
{
}

void RendererOmniShadows::SetShaders()
{
	MoravaShader* shaderOmniShadow = new MoravaShader("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("omniShadow", shaderOmniShadow));
	Log::GetLogger()->info("RendererOmniShadows: shaderOmniShadow compiled [programID={0}]", shaderOmniShadow->GetProgramID());

	MoravaShader* shaderMain = new MoravaShader("Shaders/shader.vert", "Shaders/shader.frag");
	RendererBasic::GetShaders().insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererOmniShadows: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());
}

void RendererOmniShadows::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
		if (LightManager::pointLights[i].GetEnabled())
			RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
		if (LightManager::spotLights[i].GetBasePL()->GetEnabled())
			RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
}

void RendererOmniShadows::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RendererBasic::GetShaders()["omniShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	RendererBasic::GetShaders()["omniShadow"]->SetFloat3("lightPosition", light->GetPosition());
	RendererBasic::GetShaders()["omniShadow"]->SetFloat("farPlane", light->GetFarPlane());
	std::vector<glm::mat4> lightMatrices = light->CalculateLightTransform();
	for (unsigned int i = 0; i < lightMatrices.size(); i++) {
		RendererBasic::GetShaders()["omniShadow"]->SetMat4("lightMatrices[" + std::to_string(i) + "]", lightMatrices[i]);
	}
	RendererBasic::GetShaders()["omniShadow"]->Validate();

	EnableCulling();
	std::string passType = "shadow_omni";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererOmniShadows::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear();

	MoravaShader* shaderMain = (MoravaShader*)RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	shaderMain->SetMat4("model", glm::mat4(1.0f));
	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->SetInt(  "directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMain->SetFloat3( "directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMain->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMain->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMain->SetFloat3( "directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	// Point Lights
	unsigned int textureUnit = scene->GetTextureSlots()["omniShadow"];
	unsigned int offset = 0;
	shaderMain->SetInt("pointLightCount", LightManager::pointLightCount);

	for (int i = 0; i < (int)LightManager::pointLightCount; i++)
	{
		shaderMain->SetInt(  "pointLights[" + std::to_string(i) + "].base.enabled", LightManager::pointLights[i].GetEnabled());
		shaderMain->SetFloat3( "pointLights[" + std::to_string(i) + "].base.color", LightManager::pointLights[i].GetColor());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].base.ambientIntensity", LightManager::pointLights[i].GetAmbientIntensity());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].base.diffuseIntensity", LightManager::pointLights[i].GetDiffuseIntensity());
		shaderMain->SetFloat3( "pointLights[" + std::to_string(i) + "].position", LightManager::pointLights[i].GetPosition());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].constant", LightManager::pointLights[i].GetConstant());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].linear", LightManager::pointLights[i].GetLinear());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].exponent", LightManager::pointLights[i].GetExponent());

		LightManager::pointLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMain->SetInt(  "omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::pointLights[i].GetFarPlane());
	}

	// Spot Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = LightManager::pointLightCount;
	shaderMain->SetInt("spotLightCount", LightManager::spotLightCount);

	for (int i = 0; i < (int)LightManager::spotLightCount; i++)
	{
		shaderMain->SetInt(  "spotLights[" + std::to_string(i) + "].base.base.enabled", LightManager::spotLights[i].GetBasePL()->GetEnabled());
		shaderMain->SetFloat3( "spotLights[" + std::to_string(i) + "].base.base.color", LightManager::spotLights[i].GetBasePL()->GetColor());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.base.ambientIntensity", LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.base.diffuseIntensity", LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
		shaderMain->SetFloat3( "spotLights[" + std::to_string(i) + "].base.position", LightManager::spotLights[i].GetBasePL()->GetPosition());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.constant", LightManager::spotLights[i].GetBasePL()->GetConstant());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.linear",   LightManager::spotLights[i].GetBasePL()->GetLinear());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.exponent", LightManager::spotLights[i].GetBasePL()->GetExponent());
		shaderMain->SetFloat3( "spotLights[" + std::to_string(i) + "].direction", LightManager::spotLights[i].GetDirection());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].edge", LightManager::spotLights[i].GetEdge());

		LightManager::spotLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMain->SetInt(  "omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::spotLights[i].GetFarPlane());
	}

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->SetFloat("tilingFactor", 1.0f);
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	shaderMain->Unbind();
}

void RendererOmniShadows::BeginFrame()
{
}

void RendererOmniShadows::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	RenderOmniShadows(mainWindow, scene, projectionMatrix);
	RenderPassMain(mainWindow, scene, projectionMatrix);
}
