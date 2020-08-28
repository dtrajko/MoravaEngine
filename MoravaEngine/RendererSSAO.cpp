#include "RendererSSAO.h"
#include "Log.h"
#include "SceneSSAO.h"
#include "Application.h"

#include <random>


RendererSSAO::RendererSSAO()
{
}

void RendererSSAO::Init(Scene* scene)
{
	SetShaders();
	SetupSSAO();

	SceneSSAO* sceneSSAO = (SceneSSAO*)scene;

	sceneSSAO->gPosition           = &gPosition;
	sceneSSAO->gNormal             = &gNormal;
	sceneSSAO->gAlbedo             = &gAlbedo;
	sceneSSAO->rboDepth            = &rboDepth;

	// SSAO processing stage
	sceneSSAO->ssaoFBO             = &ssaoFBO;
	sceneSSAO->ssaoBlurFBO         = &ssaoBlurFBO;
	sceneSSAO->ssaoColorBuffer     = &ssaoColorBuffer;
	sceneSSAO->ssaoColorBufferBlur = &ssaoColorBufferBlur;

	sceneSSAO->gBuffer             = &gBuffer;
	sceneSSAO->noiseTexture        = &noiseTexture;
}

void RendererSSAO::SetShaders()
{
	Shader* shaderMain = new Shader("Shaders/shader.vert", "Shaders/shader.frag");
	shaders.insert(std::make_pair("main", shaderMain));
	Log::GetLogger()->info("RendererSSAO: shaderMain compiled [programID={0}]", shaderMain->GetProgramID());

	Shader* shaderSSAOGeometry = new Shader("Shaders/LearnOpenGL/ssao_geometry.vs", "Shaders/LearnOpenGL/ssao_geometry.fs");
	shaders.insert(std::make_pair("ssao_geometry", shaderSSAOGeometry));
	Log::GetLogger()->info("RendererSSAO: shaderSSAOGeometry compiled [programID={0}]", shaderSSAOGeometry->GetProgramID());

	Shader* shaderLightingPass = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_lighting.fs");
	shaders.insert(std::make_pair("lighting_pass", shaderLightingPass));
	Log::GetLogger()->info("RendererSSAO: shaderLightingPass compiled [programID={0}]", shaderLightingPass->GetProgramID());

	Shader* shaderSSAO = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao.fs");
	shaders.insert(std::make_pair("ssao", shaderSSAO));
	Log::GetLogger()->info("RendererSSAO: shaderSSAO compiled [programID={0}]", shaderSSAO->GetProgramID());

	Shader* shaderSSAOBlur = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_blur.fs");
	shaders.insert(std::make_pair("ssao_blur", shaderSSAOBlur));
	Log::GetLogger()->info("RendererSSAO: shaderSSAOBlur compiled [programID={0}]", shaderSSAOBlur->GetProgramID());
}

void RendererSSAO::SetupSSAO()
{
	uint32_t width = Application::Get()->GetWindow()->GetBufferWidth();
	uint32_t height = Application::Get()->GetWindow()->GetBufferHeight();

	// configure g-buffer framebuffer
	// ------------------------------
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

	// position color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// color + specular color buffer
	glGenTextures(1, &gAlbedo);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// also create framebuffer to hold SSAO processing stage 
	// -----------------------------------------------------
	glGenFramebuffers(1, &ssaoFBO);
	glGenFramebuffers(1, &ssaoBlurFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

	// SSAO color buffer
	glGenTextures(1, &ssaoColorBuffer);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Framebuffer not complete!" << std::endl;

	// and blur stage
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glGenTextures(1, &ssaoColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0;

		// scale samples s.t. they're more aligned to center of kernel
		scale = Lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// generate noise texture
	// ----------------------
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	glGenTextures(1, &noiseTexture);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// lighting info
	// -------------
	lightPos = glm::vec3(2.0f, 4.0f, -2.0f);
	lightColor = glm::vec3(0.2f, 0.2f, 0.7f);

	// shader configuration
	// --------------------
	Shader* shaderGeometryPass = shaders["ssao_geometry"];
	Shader* shaderLightingPass = shaders["lighting_pass"];
	Shader* shaderSSAO         = shaders["ssao"];
	Shader* shaderSSAOBlur     = shaders["ssao_blur"];

	shaderLightingPass->Bind();
	shaderLightingPass->setInt("gPosition", 0);
	shaderLightingPass->setInt("gNormal", 1);
	shaderLightingPass->setInt("gAlbedo", 2);
	shaderLightingPass->setInt("ssao", 3);
	shaderSSAO->Bind();
	shaderSSAO->setInt("gPosition", 0);
	shaderSSAO->setInt("gNormal", 1);
	shaderSSAO->setInt("texNoise", 2);
	shaderSSAOBlur->Bind();
	shaderSSAOBlur->setInt("ssaoInput", 0);
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

	if (LightManager::directionalLight.GetShadowMap() != nullptr)
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

	/* BEGIN SSAO Rendering */
	SceneSSAO* sceneSSAO = (SceneSSAO*)scene;

	Shader* shaderGeometryPass = shaders["ssao_geometry"];
	Shader* shaderLightingPass = shaders["lighting_pass"];
	Shader* shaderSSAO = shaders["ssao"];
	Shader* shaderSSAOBlur = shaders["ssao_blur"];

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 1. geometry pass: render scene's geometry/color data into gbuffer
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = projectionMatrix;
	glm::mat4 view = scene->GetCameraController()->CalculateViewMatrix();
	glm::mat4 model = glm::mat4(1.0f);
	shaderGeometryPass->Bind();
	shaderGeometryPass->setMat4("projection", projection);
	shaderGeometryPass->setMat4("view", view);
	// room cube
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 7.0f, 0.0f));
	model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
	shaderGeometryPass->setMat4("model", model);
	shaderGeometryPass->setInt("invertedNormals", 1); // invert normals as we're inside the cube
	scene->GetMeshes()["cube"]->Render();
	shaderGeometryPass->setInt("invertedNormals", 0);
	// backpack model on the floor
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 5.0f));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.0f));
	shaderGeometryPass->setMat4("model", model);
	sceneSSAO->modelsSSAO["backpack"]->Draw(shaderGeometryPass);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. generate SSAO texture
	// ------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderSSAO->Bind();
	shaderSSAO->setMat4("projection", projection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	scene->GetMeshes()["quad_ssao"]->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 3. blur SSAO texture to remove noise
	// ------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderSSAOBlur->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	scene->GetMeshes()["quad_ssao"]->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
	// -----------------------------------------------------------------------------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shaderLightingPass->Bind();
	// send light relevant uniforms
	glm::vec3 lightPosView = glm::vec3(scene->GetCameraController()->CalculateViewMatrix() * glm::vec4(lightPos, 1.0));
	shaderLightingPass->setVec3("light.Position", lightPosView);
	shaderLightingPass->setVec3("light.Color", lightColor);
	// Update attenuation parameters
	const float constant = 1.0f; // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
	const float linear = 0.09f;
	const float quadratic = 0.032f;
	shaderLightingPass->setFloat("light.Linear", linear);
	shaderLightingPass->setFloat("light.Quadratic", quadratic);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gAlbedo);
	glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
	glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
	scene->GetMeshes()["quad_ssao"]->Render();

	/* END SSAO Rendering */

	scene->GetSettings().enableCulling ? EnableCulling() : DisableCulling();
	std::string passType = "main";
	scene->Render(mainWindow, projectionMatrix, passType, shaders, uniforms);
}

float RendererSSAO::Lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

RendererSSAO::~RendererSSAO()
{
}
