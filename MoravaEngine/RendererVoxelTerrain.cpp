#include "RendererVoxelTerrain.h"

#include "ResourceManager.h"
#include "Log.h"
#include "Profiler.h"


RendererVoxelTerrain::RendererVoxelTerrain()
{
	SetShaders();
}

void RendererVoxelTerrain::Init(Scene* scene)
{
}

void RendererVoxelTerrain::SetShaders()
{
	Shader* shaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
	s_Shaders.insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());

	Shader* shaderRenderInstanced  = new Shader("Shaders/render_instanced.vs", "Shaders/render_instanced.fs");
	s_Shaders.insert(std::make_pair("render_instanced", shaderRenderInstanced));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderRenderInstanced compiled [programID={0}]", shaderRenderInstanced->GetProgramID());

	Shader* shaderBasic = new Shader("Shaders/basic.vs", "Shaders/basic.fs");
	s_Shaders.insert(std::make_pair("basic", shaderBasic));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderBasic compiled [programID={0}]", shaderBasic->GetProgramID());

	Shader* shaderMarchingCubes = new Shader("Shaders/marching_cubes.vs", "Shaders/marching_cubes.fs");
	s_Shaders.insert(std::make_pair("marching_cubes", shaderMarchingCubes));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderMarchingCubes compiled [programID={0}]", shaderMarchingCubes->GetProgramID());

	Shader* shaderShadowMap = new Shader("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	s_Shaders.insert(std::make_pair("shadow_map", shaderShadowMap));
	Log::GetLogger()->info("RendererEditor: shaderShadowMap compiled [programID={0}]", shaderShadowMap->GetProgramID());

	Shader* shaderOmniShadow = new Shader("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
	s_Shaders.insert(std::make_pair("omniShadow", shaderOmniShadow));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderOmniShadow compiled [programID={0}]", shaderOmniShadow->GetProgramID());
}

void RendererVoxelTerrain::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableShadows) return;
	if (!LightManager::directionalLight.GetEnabled()) return;
	if (LightManager::directionalLight.GetShadowMap() == nullptr) return;

	Shader* shaderShadowMap = s_Shaders["shadow_map"];
	shaderShadowMap->Bind();

	DirectionalLight* light = &LightManager::directionalLight;
	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	/**** BEGIN shadow_map ****/
	shaderShadowMap->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());
	shaderShadowMap->Validate();
	/**** END shadow_map ****/

	DisableCulling();
	std::string passType = "shadow_dir";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererVoxelTerrain::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
		if (LightManager::pointLights[i].GetEnabled())
			RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
		if (LightManager::spotLights[i].GetBasePL()->GetEnabled())
			RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
}

void RendererVoxelTerrain::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	s_Shaders["omniShadow"]->Bind();

	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->Write();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	s_Shaders["omniShadow"]->setVec3("lightPosition", light->GetPosition());
	s_Shaders["omniShadow"]->setFloat("farPlane", light->GetFarPlane());
	std::vector<glm::mat4> lightMatrices = light->CalculateLightTransform();
	for (unsigned int i = 0; i < lightMatrices.size(); i++) {
		s_Shaders["omniShadow"]->setMat4("lightMatrices[" + std::to_string(i) + "]", lightMatrices[i]);
	}
	s_Shaders["omniShadow"]->Validate();

	EnableCulling();
	std::string passType = "shadow_omni";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererVoxelTerrain::RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	glClearColor(s_BgColor.r, s_BgColor.g, s_BgColor.b, s_BgColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/**** BEGIN shaderMain ****/
	Shader* shaderMain = (Shader*)s_Shaders["main"];
	shaderMain->Bind();

	shaderMain->setMat4("model", glm::mat4(1.0f));
	shaderMain->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->setMat4("projection", projectionMatrix);
	shaderMain->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMain->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMain->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMain->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMain->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMain->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	unsigned int textureUnit;
	unsigned int offset;

	// Point Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = 0;
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
	if (scene->GetSettings().enableShadows)
		shaderMain->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMain->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMain->setFloat("tilingFactor", 1.0f);
	shaderMain->Validate();
	/**** END shaderMain ****/

	/**** BEGIN shaderMarchingCubes ****/
	Shader* shaderMarchingCubes = (Shader*)s_Shaders["marching_cubes"];
	shaderMarchingCubes->Bind();

	shaderMarchingCubes->setMat4("model", glm::mat4(1.0f));
	shaderMarchingCubes->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMarchingCubes->setMat4("projection", projectionMatrix);
	shaderMarchingCubes->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMarchingCubes->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMarchingCubes->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMarchingCubes->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMarchingCubes->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMarchingCubes->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMarchingCubes->setMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	// Point Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = 0;
	shaderMarchingCubes->setInt("pointLightCount", LightManager::pointLightCount);

	for (int i = 0; i < (int)LightManager::pointLightCount; i++)
	{
		shaderMarchingCubes->setInt("pointLights[" + std::to_string(i) + "].base.enabled", LightManager::pointLights[i].GetEnabled());
		shaderMarchingCubes->setVec3("pointLights[" + std::to_string(i) + "].base.color", LightManager::pointLights[i].GetColor());
		shaderMarchingCubes->setFloat("pointLights[" + std::to_string(i) + "].base.ambientIntensity", LightManager::pointLights[i].GetAmbientIntensity());
		shaderMarchingCubes->setFloat("pointLights[" + std::to_string(i) + "].base.diffuseIntensity", LightManager::pointLights[i].GetDiffuseIntensity());
		shaderMarchingCubes->setVec3("pointLights[" + std::to_string(i) + "].position", LightManager::pointLights[i].GetPosition());
		shaderMarchingCubes->setFloat("pointLights[" + std::to_string(i) + "].constant", LightManager::pointLights[i].GetConstant());
		shaderMarchingCubes->setFloat("pointLights[" + std::to_string(i) + "].linear", LightManager::pointLights[i].GetLinear());
		shaderMarchingCubes->setFloat("pointLights[" + std::to_string(i) + "].exponent", LightManager::pointLights[i].GetExponent());

		LightManager::pointLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		shaderMarchingCubes->setInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMarchingCubes->setFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::pointLights[i].GetFarPlane());
	}

	// Spot Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = LightManager::pointLightCount;
	shaderMarchingCubes->setInt("spotLightCount", LightManager::spotLightCount);

	for (int i = 0; i < (int)LightManager::spotLightCount; i++)
	{
		shaderMarchingCubes->setInt("spotLights[" + std::to_string(i) + "].base.base.enabled", LightManager::spotLights[i].GetBasePL()->GetEnabled());
		shaderMarchingCubes->setVec3("spotLights[" + std::to_string(i) + "].base.base.color", LightManager::spotLights[i].GetBasePL()->GetColor());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].base.base.ambientIntensity", LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].base.base.diffuseIntensity", LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
		shaderMarchingCubes->setVec3("spotLights[" + std::to_string(i) + "].base.position", LightManager::spotLights[i].GetBasePL()->GetPosition());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].base.constant", LightManager::spotLights[i].GetBasePL()->GetConstant());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].base.linear", LightManager::spotLights[i].GetBasePL()->GetLinear());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].base.exponent", LightManager::spotLights[i].GetBasePL()->GetExponent());
		shaderMarchingCubes->setVec3("spotLights[" + std::to_string(i) + "].direction", LightManager::spotLights[i].GetDirection());
		shaderMarchingCubes->setFloat("spotLights[" + std::to_string(i) + "].edge", LightManager::spotLights[i].GetEdge());

		LightManager::spotLights[i].GetShadowMap()->Read(textureUnit + offset + i);
		shaderMarchingCubes->setInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMarchingCubes->setFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::spotLights[i].GetFarPlane());
	}

	LightManager::directionalLight.GetShadowMap()->Read(scene->GetTextureSlots()["shadow"]);
	shaderMarchingCubes->setInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMarchingCubes->setInt("normalMap", scene->GetTextureSlots()["normal"]);
	if (scene->GetSettings().enableShadows)
		shaderMarchingCubes->setInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	shaderMarchingCubes->setVec4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, -10000));
	shaderMarchingCubes->setFloat("tilingFactor", 1.0f);
	shaderMarchingCubes->Validate();
	/**** END shaderMarchingCubes ****/

	/**** BEGIN shaderRenderInstanced ****/
	Shader* shaderRenderInstanced = (Shader*)s_Shaders["render_instanced"];
	shaderRenderInstanced->Bind();

	shaderRenderInstanced->setMat4("projection", projectionMatrix);
	shaderRenderInstanced->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderRenderInstanced->setVec3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderRenderInstanced->setInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderRenderInstanced->setVec3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderRenderInstanced->setFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderRenderInstanced->setFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderRenderInstanced->setVec3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderRenderInstanced->setFloat("material.specularIntensity", ResourceManager::s_MaterialSpecular);  // TODO - use material attribute
	shaderRenderInstanced->setFloat("material.shininess", ResourceManager::s_MaterialShininess); // TODO - use material attribute
	shaderRenderInstanced->Validate();
	/**** END shaderRenderInstanced ****/

	/**** BEGIN shaderBasic ****/
	Shader* shaderBasic = s_Shaders["basic"];
	shaderBasic->Bind();
	shaderBasic->setMat4("projection", projectionMatrix);
	shaderBasic->setMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderBasic->Validate();
	/**** END shaderBasic ****/

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, s_Shaders, s_Uniforms);
}

void RendererVoxelTerrain::Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	{
		Profiler profiler("RVT::RenderPassShadow");
		RenderPassShadow(mainWindow, scene, projectionMatrix);
		scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
	}

	// RenderOmniShadows(mainWindow, scene, projectionMatrix);

	{
		Profiler profiler("RVT::RenderPass");
		RenderPass(mainWindow, scene, projectionMatrix);
		scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
	}
}

RendererVoxelTerrain::~RendererVoxelTerrain()
{
}
