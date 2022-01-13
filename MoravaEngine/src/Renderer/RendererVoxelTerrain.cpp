#include "Renderer/RendererVoxelTerrain.h"

#include "Core/Profiler.h"
#include "Core/ResourceManager.h"
#include "Shader/ShaderMain.h"


RendererVoxelTerrain::RendererVoxelTerrain()
{
	SetUniforms();
	SetShaders();
}

RendererVoxelTerrain::~RendererVoxelTerrain()
{
}

void RendererVoxelTerrain::Init(Scene* scene)
{
}

void RendererVoxelTerrain::SetUniforms()
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

void RendererVoxelTerrain::SetShaders()
{
	H2M::RefH2M<MoravaShader> shaderMain = MoravaShader::Create("Shaders/shader.vert", "Shaders/shader.frag");
	RendererBasic::GetShaders().insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderRenderInstanced  = MoravaShader::Create("Shaders/render_instanced.vs", "Shaders/render_instanced.fs");
	RendererBasic::GetShaders().insert(std::make_pair("render_instanced", shaderRenderInstanced));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderRenderInstanced compiled [programID={0}]", shaderRenderInstanced->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderBasic = MoravaShader::Create("Shaders/basic.vs", "Shaders/basic.fs");
	RendererBasic::GetShaders().insert(std::make_pair("basic", shaderBasic));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderBasic compiled [programID={0}]", shaderBasic->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderMarchingCubes = MoravaShader::Create("Shaders/marching_cubes.vs", "Shaders/marching_cubes.fs");
	RendererBasic::GetShaders().insert(std::make_pair("marching_cubes", shaderMarchingCubes));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderMarchingCubes compiled [programID={0}]", shaderMarchingCubes->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderShadowMap = MoravaShader::Create("Shaders/directional_shadow_map.vert", "Shaders/directional_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("shadow_map", shaderShadowMap));
	Log::GetLogger()->info("RendererEditor: shaderShadowMap compiled [programID={0}]", shaderShadowMap->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderOmniShadow = MoravaShader::Create("Shaders/omni_shadow_map.vert", "Shaders/omni_shadow_map.geom", "Shaders/omni_shadow_map.frag");
	RendererBasic::GetShaders().insert(std::make_pair("omniShadow", shaderOmniShadow));
	Log::GetLogger()->info("RendererVoxelTerrain: shaderOmniShadow compiled [programID={0}]", shaderOmniShadow->GetProgramID());

	H2M::RefH2M<MoravaShader> shaderWater = MoravaShader::Create("Shaders/water.vert", "Shaders/water.frag");
	RendererBasic::GetShaders().insert(std::make_pair("water", shaderWater));
	printf("Renderer: Water shader compiled [programID=%d]\n", shaderWater->GetProgramID());
}

// BEGIN shadow render passes
void RendererVoxelTerrain::RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableShadows) return;
	if (!LightManager::directionalLight.GetEnabled()) return;
	if (LightManager::directionalLight.GetShadowMap() == nullptr) return;

	H2M::RefH2M<MoravaShader> shaderShadowMap = RendererBasic::GetShaders()["shadow_map"];
	shaderShadowMap->Bind();

	DirectionalLight* light = &LightManager::directionalLight;
	glViewport(0, 0, light->GetShadowMap()->GetShadowWidth(), light->GetShadowMap()->GetShadowHeight());

	light->GetShadowMap()->BindForWriting();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);

	/**** BEGIN shadow_map ****/
	shaderShadowMap->SetMat4("u_DirLightTransform", LightManager::directionalLight.CalculateLightTransform());
	shaderShadowMap->SetBool("u_Animated", false);
	shaderShadowMap->Validate();
	/**** END shadow_map ****/

	DisableCulling();
	std::string passType = "shadow_dir";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererVoxelTerrain::RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

	for (size_t i = 0; i < LightManager::pointLightCount; i++)
	{
		if (LightManager::pointLights[i].GetEnabled())
		{
			RenderPassOmniShadow(&LightManager::pointLights[i], mainWindow, scene, projectionMatrix);
		}
	}

	for (size_t i = 0; i < LightManager::spotLightCount; i++)
	{
		if (LightManager::spotLights[i].GetBasePL()->GetEnabled())
		{
			RenderPassOmniShadow((PointLight*)&LightManager::spotLights[i], mainWindow, scene, projectionMatrix);
		}
	}
}

void RendererVoxelTerrain::RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableOmniShadows) return;

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
// END shadow render passes

// BEGIN water render passes
void RendererVoxelTerrain::RenderWaterEffects(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	if (!scene->GetSettings().enableWaterEffects) return;

	glEnable(GL_CLIP_DISTANCE0);

	float waterMoveFactor = scene->GetWaterManager()->GetWaterMoveFactor();
	waterMoveFactor += WaterManager::m_WaveSpeed * deltaTime;
	if (waterMoveFactor >= 1.0f)
	{
		waterMoveFactor = waterMoveFactor - 1.0f;
	}
	scene->GetWaterManager()->SetWaterMoveFactor(waterMoveFactor);

	float distance = 2.0f * (scene->GetCamera()->GetPosition().y - scene->GetSettings().waterHeight);
	glm::vec3 cameraPosition = scene->GetCamera()->GetPosition();
	glm::vec3 cameraPositionInverse = scene->GetCamera()->GetPosition();
	cameraPositionInverse.y -= distance;
	scene->GetCamera()->SetPosition(cameraPositionInverse);
	scene->GetCameraController()->InvertPitch();

	RenderPassWaterReflection(mainWindow, scene, projectionMatrix);

	scene->GetCamera()->SetPosition(cameraPosition);
	scene->GetCameraController()->InvertPitch();

	RenderPassWaterRefraction(mainWindow, scene, projectionMatrix);
}

void RendererVoxelTerrain::RenderPassWaterReflection(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetReflectionFramebuffer()->Bind();

	// Clear the window																									
	RendererBasic::Clear();

	H2M::RefH2M<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"] = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"] = shaderMain->GetUniformLocation("view");
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
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetSettings().waterHeight)); // reflection clip plane
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	H2M::RefH2M<MoravaShader> shaderRenderInstanced = RendererBasic::GetShaders()["render_instanced"];
	shaderRenderInstanced->Bind();
	shaderRenderInstanced->SetFloat4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, -scene->GetSettings().waterHeight)); // reflection clip plane
	shaderRenderInstanced->Validate();

	EnableCulling();
	std::string passType = "water";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RendererVoxelTerrain::RenderPassWaterRefraction(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glViewport(0, 0, scene->GetWaterManager()->GetFramebufferWidth(), scene->GetWaterManager()->GetFramebufferHeight());

	scene->GetWaterManager()->GetRefractionFramebuffer()->Bind();
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetColorAttachment()->Bind(scene->GetTextureSlots()["refraction"]);
	scene->GetWaterManager()->GetRefractionFramebuffer()->GetDepthAttachment()->Bind(scene->GetTextureSlots()["depth"]);

	// Clear the window
	RendererBasic::Clear();

	H2M::RefH2M<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"] = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"] = shaderMain->GetUniformLocation("view");
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
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetSettings().waterHeight)); // refraction clip plane
	shaderMain->SetFloat4("tintColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	shaderMain->Validate();

	H2M::RefH2M<MoravaShader> shaderRenderInstanced = RendererBasic::GetShaders()["render_instanced"];
	shaderRenderInstanced->Bind();
	shaderRenderInstanced->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, scene->GetSettings().waterHeight)); // refraction clip plane
	shaderRenderInstanced->Validate();

	std::string passType = "water";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
// END water render passes

void RendererVoxelTerrain::RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	glDisable(GL_CLIP_DISTANCE0);

	glViewport(0, 0, (GLsizei)mainWindow->GetWidth(), (GLsizei)mainWindow->GetHeight());

	// Clear the window
	RendererBasic::Clear();

	/**** BEGIN shaderMain ****/
	H2M::RefH2M<ShaderMain> shaderMain = RendererBasic::GetShaders()["main"];
	shaderMain->Bind();

	RendererBasic::GetUniforms()["model"] = shaderMain->GetUniformLocation("model");
	RendererBasic::GetUniforms()["projection"] = shaderMain->GetUniformLocation("projection");
	RendererBasic::GetUniforms()["view"] = shaderMain->GetUniformLocation("view");
	RendererBasic::GetUniforms()["eyePosition"] = shaderMain->GetUniformLocation("eyePosition");
	RendererBasic::GetUniforms()["specularIntensity"] = shaderMain->GetUniformLocationMaterialSpecularIntensity();
	RendererBasic::GetUniforms()["shininess"] = shaderMain->GetUniformLocationMaterialShininess();

	shaderMain->SetMat4("model", glm::mat4(1.0f));
	shaderMain->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMain->SetMat4("projection", projectionMatrix);
	shaderMain->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMain->SetInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMain->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMain->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMain->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMain->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMain->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	unsigned int textureUnit;
	unsigned int offset;

	// Point Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = 0;
	shaderMain->SetInt("pointLightCount", LightManager::pointLightCount);

	for (int i = 0; i < (int)LightManager::pointLightCount; i++)
	{
		shaderMain->SetInt("pointLights[" + std::to_string(i) + "].base.enabled", LightManager::pointLights[i].GetEnabled());
		shaderMain->SetFloat3("pointLights[" + std::to_string(i) + "].base.color", LightManager::pointLights[i].GetColor());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].base.ambientIntensity", LightManager::pointLights[i].GetAmbientIntensity());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].base.diffuseIntensity", LightManager::pointLights[i].GetDiffuseIntensity());
		shaderMain->SetFloat3("pointLights[" + std::to_string(i) + "].position", LightManager::pointLights[i].GetPosition());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].constant", LightManager::pointLights[i].GetConstant());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].linear", LightManager::pointLights[i].GetLinear());
		shaderMain->SetFloat("pointLights[" + std::to_string(i) + "].exponent", LightManager::pointLights[i].GetExponent());

		LightManager::pointLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMain->SetInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::pointLights[i].GetFarPlane());
	}

	// Spot Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = LightManager::pointLightCount;
	shaderMain->SetInt("spotLightCount", LightManager::spotLightCount);

	for (int i = 0; i < (int)LightManager::spotLightCount; i++)
	{
		shaderMain->SetInt("spotLights[" + std::to_string(i) + "].base.base.enabled", LightManager::spotLights[i].GetBasePL()->GetEnabled());
		shaderMain->SetFloat3("spotLights[" + std::to_string(i) + "].base.base.color", LightManager::spotLights[i].GetBasePL()->GetColor());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.base.ambientIntensity", LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.base.diffuseIntensity", LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
		shaderMain->SetFloat3("spotLights[" + std::to_string(i) + "].base.position", LightManager::spotLights[i].GetBasePL()->GetPosition());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.constant", LightManager::spotLights[i].GetBasePL()->GetConstant());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.linear", LightManager::spotLights[i].GetBasePL()->GetLinear());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].base.exponent", LightManager::spotLights[i].GetBasePL()->GetExponent());
		shaderMain->SetFloat3("spotLights[" + std::to_string(i) + "].direction", LightManager::spotLights[i].GetDirection());
		shaderMain->SetFloat("spotLights[" + std::to_string(i) + "].edge", LightManager::spotLights[i].GetEdge());

		LightManager::spotLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMain->SetInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMain->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::spotLights[i].GetFarPlane());
	}

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMain->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMain->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	if (scene->GetSettings().enableShadows)
	{
		shaderMain->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	}

	// clip plane for rendering to screen
	shaderMain->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, 10000.0f));

	shaderMain->SetFloat("tilingFactor", 1.0f);
	shaderMain->Validate();
	/**** END shaderMain ****/

	/**** BEGIN shaderMarchingCubes ****/
	H2M::RefH2M<MoravaShader> shaderMarchingCubes = RendererBasic::GetShaders()["marching_cubes"];
	shaderMarchingCubes->Bind();

	shaderMarchingCubes->SetMat4("model", glm::mat4(1.0f));
	shaderMarchingCubes->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderMarchingCubes->SetMat4("projection", projectionMatrix);
	shaderMarchingCubes->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderMarchingCubes->SetInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderMarchingCubes->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderMarchingCubes->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderMarchingCubes->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderMarchingCubes->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderMarchingCubes->SetMat4("dirLightTransform", LightManager::directionalLight.CalculateLightTransform());

	// Point Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = 0;
	shaderMarchingCubes->SetInt("pointLightCount", LightManager::pointLightCount);

	for (int i = 0; i < (int)LightManager::pointLightCount; i++)
	{
		shaderMarchingCubes->SetInt("pointLights[" + std::to_string(i) + "].base.enabled", LightManager::pointLights[i].GetEnabled());
		shaderMarchingCubes->SetFloat3("pointLights[" + std::to_string(i) + "].base.color", LightManager::pointLights[i].GetColor());
		shaderMarchingCubes->SetFloat("pointLights[" + std::to_string(i) + "].base.ambientIntensity", LightManager::pointLights[i].GetAmbientIntensity());
		shaderMarchingCubes->SetFloat("pointLights[" + std::to_string(i) + "].base.diffuseIntensity", LightManager::pointLights[i].GetDiffuseIntensity());
		shaderMarchingCubes->SetFloat3("pointLights[" + std::to_string(i) + "].position", LightManager::pointLights[i].GetPosition());
		shaderMarchingCubes->SetFloat("pointLights[" + std::to_string(i) + "].constant", LightManager::pointLights[i].GetConstant());
		shaderMarchingCubes->SetFloat("pointLights[" + std::to_string(i) + "].linear", LightManager::pointLights[i].GetLinear());
		shaderMarchingCubes->SetFloat("pointLights[" + std::to_string(i) + "].exponent", LightManager::pointLights[i].GetExponent());

		LightManager::pointLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMarchingCubes->SetInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMarchingCubes->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::pointLights[i].GetFarPlane());
	}

	// Spot Lights
	textureUnit = scene->GetTextureSlots()["omniShadow"];
	offset = LightManager::pointLightCount;
	shaderMarchingCubes->SetInt("spotLightCount", LightManager::spotLightCount);

	for (int i = 0; i < (int)LightManager::spotLightCount; i++)
	{
		shaderMarchingCubes->SetInt("spotLights[" + std::to_string(i) + "].base.base.enabled", LightManager::spotLights[i].GetBasePL()->GetEnabled());
		shaderMarchingCubes->SetFloat3("spotLights[" + std::to_string(i) + "].base.base.color", LightManager::spotLights[i].GetBasePL()->GetColor());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].base.base.ambientIntensity", LightManager::spotLights[i].GetBasePL()->GetAmbientIntensity());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].base.base.diffuseIntensity", LightManager::spotLights[i].GetBasePL()->GetDiffuseIntensity());
		shaderMarchingCubes->SetFloat3("spotLights[" + std::to_string(i) + "].base.position", LightManager::spotLights[i].GetBasePL()->GetPosition());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].base.constant", LightManager::spotLights[i].GetBasePL()->GetConstant());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].base.linear", LightManager::spotLights[i].GetBasePL()->GetLinear());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].base.exponent", LightManager::spotLights[i].GetBasePL()->GetExponent());
		shaderMarchingCubes->SetFloat3("spotLights[" + std::to_string(i) + "].direction", LightManager::spotLights[i].GetDirection());
		shaderMarchingCubes->SetFloat("spotLights[" + std::to_string(i) + "].edge", LightManager::spotLights[i].GetEdge());

		LightManager::spotLights[i].GetShadowMap()->ReadTexture(textureUnit + offset + i);
		shaderMarchingCubes->SetInt("omniShadowMaps[" + std::to_string(offset + i) + "].shadowMap", textureUnit + offset + i);
		shaderMarchingCubes->SetFloat("omniShadowMaps[" + std::to_string(offset + i) + "].farPlane", LightManager::spotLights[i].GetFarPlane());
	}

	LightManager::directionalLight.GetShadowMap()->ReadTexture(scene->GetTextureSlots()["shadow"]);
	shaderMarchingCubes->SetInt("albedoMap", scene->GetTextureSlots()["diffuse"]);
	shaderMarchingCubes->SetInt("normalMap", scene->GetTextureSlots()["normal"]);
	if (scene->GetSettings().enableShadows)
	{
		shaderMarchingCubes->SetInt("shadowMap", scene->GetTextureSlots()["shadow"]);
	}

	// clip plane for rendering to screen
	shaderMarchingCubes->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, 10000.0f));

	shaderMarchingCubes->SetFloat("tilingFactor", 1.0f);
	shaderMarchingCubes->Validate();
	/**** END shaderMarchingCubes ****/

	/**** BEGIN shaderRenderInstanced ****/
	H2M::RefH2M<MoravaShader> shaderRenderInstanced = RendererBasic::GetShaders()["render_instanced"];
	shaderRenderInstanced->Bind();

	shaderRenderInstanced->SetMat4("projection", projectionMatrix);
	shaderRenderInstanced->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderRenderInstanced->SetFloat3("eyePosition", scene->GetCamera()->GetPosition());

	// Directional Light
	shaderRenderInstanced->SetInt("directionalLight.base.enabled", LightManager::directionalLight.GetEnabled());
	shaderRenderInstanced->SetFloat3("directionalLight.base.color", LightManager::directionalLight.GetColor());
	shaderRenderInstanced->SetFloat("directionalLight.base.ambientIntensity", LightManager::directionalLight.GetAmbientIntensity());
	shaderRenderInstanced->SetFloat("directionalLight.base.diffuseIntensity", LightManager::directionalLight.GetDiffuseIntensity());
	shaderRenderInstanced->SetFloat3("directionalLight.direction", LightManager::directionalLight.GetDirection());

	shaderRenderInstanced->SetFloat("material.specularIntensity", ResourceManager::s_MaterialSpecular);  // TODO - use material attribute
	shaderRenderInstanced->SetFloat("material.shininess", ResourceManager::s_MaterialShininess); // TODO - use material attribute

	// clip plane for rendering to screen
	shaderRenderInstanced->SetFloat4("clipPlane", glm::vec4(0.0f, -1.0f, 0.0f, 10000.0f));

	shaderRenderInstanced->Validate();
	/**** END shaderRenderInstanced ****/

	/**** BEGIN shaderBasic ****/
	H2M::RefH2M<MoravaShader> shaderBasic = RendererBasic::GetShaders()["basic"];
	shaderBasic->Bind();
	shaderBasic->SetMat4("projection", projectionMatrix);
	shaderBasic->SetMat4("view", scene->GetCamera()->GetViewMatrix());
	shaderBasic->Validate();
	/**** END shaderBasic ****/

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, RendererBasic::GetShaders(), RendererBasic::GetUniforms());

	// shaderMain->Unbind();
	// shaderMarchingCubes->Unbind();
	// shaderRenderInstanced->Unbind();
	// shaderBasic->Unbind();

	/**** BEGIN render water ****/
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
	shaderWater->SetFloat("nearPlane", scene->GetSettings().nearPlane);
	shaderWater->SetFloat("farPlane", scene->GetSettings().farPlane);
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
	/**** END render water ****/
}

void RendererVoxelTerrain::BeginFrame()
{
}

void RendererVoxelTerrain::WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix)
{
	{
		Profiler profiler("RVT::RenderPassShadow");
		RenderPassShadow(mainWindow, scene, projectionMatrix);
		scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
	}

	// RenderOmniShadows(mainWindow, scene, projectionMatrix);

	{
		Profiler profiler("RVT::RenderWaterEffects");
		RenderWaterEffects(deltaTime, mainWindow, scene, projectionMatrix);
	}

	{
		Profiler profiler("RVT::RenderPass");
		RenderPassMain(mainWindow, scene, projectionMatrix);
		scene->GetProfilerResults()->insert(std::make_pair(profiler.GetName(), profiler.Stop()));
	}
}
