#include "SSAO.h"

#include "Log.h"
#include "LearnOpenGL/ModelSSAO.h"
#include "Math.h"
#include "Mesh.h"
#include "Timer.h"

#include "Core/Application.h"

#include <random>


SSAO::SSAO()
{
	ResetHandlers();

	m_UpdateCooldown = { 0.0f, 0.2f };

	kernelSize = 64;
	radius = 0.5f;
	bias = 0.025f;
}

void SSAO::SetupShaders()
{
	shaderGeometryPass = new Shader("Shaders/LearnOpenGL/ssao_geometry.vs", "Shaders/LearnOpenGL/ssao_geometry.fs");
	Log::GetLogger()->info("SSAO: shaderGeometryPass compiled [programID={0}]", shaderGeometryPass->GetProgramID());

	shaderLightingPass = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_lighting.fs");
	Log::GetLogger()->info("SSAO: shaderLightingPass compiled [programID={0}]", shaderLightingPass->GetProgramID());

	shaderSSAO = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao.fs");
	Log::GetLogger()->info("SSAO: shaderSSAO compiled [programID={0}]", shaderSSAO->GetProgramID());

	shaderSSAOBlur = new Shader("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_blur.fs");
	Log::GetLogger()->info("SSAO: shaderSSAOBlur compiled [programID={0}]", shaderSSAOBlur->GetProgramID());
}

void SSAO::Release()
{
	Log::GetLogger()->info("SSAO::Release");

	if (gBuffer)
		glDeleteFramebuffers(1, &gBuffer);
	if (ssaoFBO)
		glDeleteFramebuffers(1, &ssaoFBO);
	if (ssaoBlurFBO)
		glDeleteFramebuffers(1, &ssaoBlurFBO);

	if (gPosition)
		glDeleteTextures(1, &gPosition);
	if (gNormal)
		glDeleteTextures(1, &gNormal);
	if (gAlbedo)
		glDeleteTextures(1, &gAlbedo);

	if (rboDepth)
		glDeleteRenderbuffers(1, &rboDepth);

	if (ssaoColorBuffer)
		glDeleteTextures(1, &ssaoColorBuffer);
	if (ssaoColorBufferBlur)
		glDeleteTextures(1, &ssaoColorBufferBlur);
	if (noiseTexture)
		glDeleteTextures(1, &noiseTexture);

	ResetHandlers();
}

void SSAO::ResetHandlers()
{
	// Framebuffers
	gBuffer = 0;
	ssaoFBO = 0;
	ssaoBlurFBO = 0;

	// textures / framebuffer attachments
	gPosition = 0;
	gNormal = 0;
	gAlbedo = 0;
	rboDepth = 0;
	ssaoColorBuffer = 0;
	ssaoColorBufferBlur = 0;
	noiseTexture = 0;
}

void SSAO::Generate(unsigned int width, unsigned int height)
{
	Release();

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
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = Math::Lerp(0.1f, 1.0f, scale * scale);
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
	shaderLightingPass->Bind();
	shaderLightingPass->setInt("gPosition", 0);
	shaderLightingPass->setInt("gNormal", 1);
	shaderLightingPass->setInt("gAlbedo", 2);
	shaderLightingPass->setInt("ssao", 3);
	shaderSSAO->Bind();
	shaderSSAO->setInt("gPosition", 0);
	shaderSSAO->setInt("gNormal", 1);
	shaderSSAO->setInt("texNoise", 2);
	shaderSSAO->setFloat("screenWidth", (float)width);
	shaderSSAO->setFloat("screenHeight", (float)height);
	shaderSSAOBlur->Bind();
	shaderSSAOBlur->setInt("ssaoInput", 0);
}

void SSAO::Init()
{
	SetupShaders();

	uint32_t width = Application::Get()->GetWindow()->GetWidth();
	uint32_t height = Application::Get()->GetWindow()->GetHeight();

	m_WidthPrev = width;
	m_HeightPrev = height;

	Generate(width, height);
}

void SSAO::Update(float timestep)
{
	UpdateCooldown(timestep);
}

void SSAO::UpdateCooldown(float timestep)
{
	// Cooldown
	if (timestep - m_UpdateCooldown.lastTime < m_UpdateCooldown.cooldown) return;
	m_UpdateCooldown.lastTime = timestep;

	uint32_t width = Application::Get()->GetWindow()->GetWidth();
	uint32_t height = Application::Get()->GetWindow()->GetHeight();

	if (width != m_WidthPrev || height != m_HeightPrev)
	{
		Generate(width, height);

		m_WidthPrev = width;
		m_HeightPrev = height;
	}
}

void SSAO::Render(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
	std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models)
{
	lightColor = LightManager::directionalLight.GetColor();

	// BEGIN SSAO Rendering
	// -----------------------------------------------------------------

	// 1. geometry pass: render scene's geometry/color data into gbuffer
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = projectionMatrix;
	glm::mat4 view = viewMatrix;
	glm::mat4 model = glm::mat4(1.0f);
	shaderGeometryPass->Bind();
	shaderGeometryPass->setMat4("projection", projection);
	shaderGeometryPass->setMat4("view", view);

	// room cube
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0, 16.0f, 0.0f));
	model = glm::scale(model, glm::vec3(16.0f, 16.0f, 16.0f));
	shaderGeometryPass->setMat4("model", model);
	shaderGeometryPass->setInt("invertedNormals", 1); // invert normals as we're inside the cube
	// RenderCube();
	meshes["cube"]->Render();
	shaderGeometryPass->setInt("invertedNormals", 0);

	// backpack model on the floor
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 6.6f, -2.15));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
	model = glm::scale(model, glm::vec3(1.0f));
	shaderGeometryPass->setMat4("model", model);
	(*models)["backpack"]->Draw(shaderGeometryPass);

	// nanosuit model
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	// model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.62f));
	shaderGeometryPass->setMat4("model", model);
	(*models)["nanosuit"]->Draw(shaderGeometryPass);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. generate SSAO texture
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderSSAO->Bind();

	shaderSSAO->setInt("kernelSize", kernelSize);
	shaderSSAO->setFloat("radius", radius);
	shaderSSAO->setFloat("bias", bias);

	// Send kernel + rotation 
	for (unsigned int i = 0; i < kernelSize; ++i)
		shaderSSAO->setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
	shaderSSAO->setMat4("projection", projection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
	// RenderQuad();
	meshes["quad_ssao"]->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 3. blur SSAO texture to remove noise
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
	glClear(GL_COLOR_BUFFER_BIT);
	shaderSSAOBlur->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
	// RenderQuad();
	meshes["quad_ssao"]->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
	// -----------------------------------------------------------------------------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	shaderLightingPass->Bind();
	// send light relevant uniforms
	glm::vec3 lightPosView = glm::vec3(viewMatrix * glm::vec4(lightPos, 1.0));
	shaderLightingPass->setVec3("light.Position", lightPosView);
	shaderLightingPass->setVec3("light.Color", lightColor);
	// Update attenuation parameters
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
	// RenderQuad();
	meshes["quad_ssao"]->Render();

	// -----------------------------------------------------------------
	// END SSAO Rendering
}

SSAO::~SSAO()
{
	Release();
}
