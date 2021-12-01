#include "SSAO.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Math.h"
#include "Core/Timer.h"
#include "LearnOpenGL/ModelSSAO.h"
#include "Mesh/Mesh.h"

#include <random>


SSAO::SSAO()
{
	ResetHandlers();

	m_KernelSize = 64;
	m_KernelRadius = 0.5f;
	m_KernelBias = 0.025f;

	m_BlurEnabled = true;

	m_WidthPrev = 0;
	m_HeightPrev = 0;
}

SSAO::~SSAO()
{
	Release();
}

void SSAO::Init()
{
	SetupShaders();

	m_GBufferSSAO.Init();
	GenerateConditional();

	m_QuadSSAO = H2M::RefH2M<QuadSSAO>::Create();
}

void SSAO::SetupShaders()
{
	m_ShaderGeometryPass = MoravaShader::Create("Shaders/LearnOpenGL/ssao_geometry.vs", "Shaders/LearnOpenGL/ssao_geometry.fs");
	Log::GetLogger()->info("SSAO: shaderGeometryPass compiled [programID={0}]", m_ShaderGeometryPass->GetProgramID());

	m_ShaderLightingPass = MoravaShader::Create("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_lighting.fs");
	Log::GetLogger()->info("SSAO: shaderLightingPass compiled [programID={0}]", m_ShaderLightingPass->GetProgramID());

	m_ShaderSSAO = MoravaShader::Create("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao.fs");
	Log::GetLogger()->info("SSAO: shaderSSAO compiled [programID={0}]", m_ShaderSSAO->GetProgramID());

	m_ShaderSSAOBlur = MoravaShader::Create("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao_blur.fs");
	Log::GetLogger()->info("SSAO: shaderSSAOBlur compiled [programID={0}]", m_ShaderSSAOBlur->GetProgramID());
}

void SSAO::Release()
{
	Log::GetLogger()->info("SSAO::Release");

	if (m_SSAO_FBO)
		glDeleteFramebuffers(1, &m_SSAO_FBO);
	if (m_SSAO_FBO_Blur)
		glDeleteFramebuffers(1, &m_SSAO_FBO_Blur);

	if (m_SSAO_ColorBuffer)
		glDeleteTextures(1, &m_SSAO_ColorBuffer);
	if (m_SSAO_ColorBufferBlur)
		glDeleteTextures(1, &m_SSAO_ColorBufferBlur);
	if (m_NoiseTexture)
		glDeleteTextures(1, &m_NoiseTexture);

	ResetHandlers();
}

void SSAO::ResetHandlers()
{
	// Framebuffers
	m_SSAO_FBO = 0;
	m_SSAO_FBO_Blur = 0;

	// textures / framebuffer attachments
	m_SSAO_ColorBuffer = 0;
	m_SSAO_ColorBufferBlur = 0;
	m_NoiseTexture = 0;
}

void SSAO::GenerateConditional()
{
	m_Width = Application::Get()->GetWindow()->GetWidth();
	m_Height = Application::Get()->GetWindow()->GetHeight();

	if (m_Width != m_WidthPrev || m_Height != m_HeightPrev)
	{
		Generate();

		m_WidthPrev = m_Width;
		m_HeightPrev = m_Height;
	}
}

void SSAO::Generate()
{
	Log::GetLogger()->warn("SSAO::Generate - width: {0}, height: {1}", m_Width, m_Height);

	Release();

	GenerateSSAO_FBO();
	GenerateSSAO_BlurFBO();
	GenerateSampleKernel();
	GenerateNoiseTexture();
	GenerateLightingInfo();
}

void SSAO::GenerateSSAO_FBO()
{
	// also create framebuffer to hold SSAO processing stage 
	// -----------------------------------------------------
	glGenFramebuffers(1, &m_SSAO_FBO);
	glGenFramebuffers(1, &m_SSAO_FBO_Blur);
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO);

	// SSAO color buffer
	glGenTextures(1, &m_SSAO_ColorBuffer);
	glBindTexture(GL_TEXTURE_2D, m_SSAO_ColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAO_ColorBuffer, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Framebuffer not complete!" << std::endl;
}

void SSAO::GenerateSSAO_BlurFBO()
{
	// and blur stage
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO_Blur);
	glGenTextures(1, &m_SSAO_ColorBufferBlur);
	glBindTexture(GL_TEXTURE_2D, m_SSAO_ColorBufferBlur);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAO_ColorBufferBlur, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::GenerateSampleKernel()
{
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
		m_SSAO_Kernel.push_back(sample);
	}
}

void SSAO::GenerateNoiseTexture()
{
	// generate noise texture
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<glm::vec3> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	glGenTextures(1, &m_NoiseTexture);
	glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAO::GenerateLightingInfo()
{
	// lighting info
	// -------------
	m_LightPos = glm::vec3(2.0f, 4.0f, -2.0f);
	m_LightColor = glm::vec3(0.2f, 0.2f, 0.7f);

	// shader configuration
	// --------------------
	m_ShaderLightingPass->Bind();
	m_ShaderLightingPass->SetInt("gPosition", 0);
	m_ShaderLightingPass->SetInt("gNormal", 1);
	m_ShaderLightingPass->SetInt("gAlbedo", 2);
	m_ShaderLightingPass->SetInt("ssao", 3);
	m_ShaderSSAO->Bind();
	m_ShaderSSAO->SetInt("gPosition", 0);
	m_ShaderSSAO->SetInt("gNormal", 1);
	m_ShaderSSAO->SetInt("texNoise", 2);
	m_ShaderSSAO->SetFloat("screenWidth", (float)m_Width);
	m_ShaderSSAO->SetFloat("screenHeight", (float)m_Height);
	m_ShaderSSAOBlur->Bind();
	m_ShaderSSAOBlur->SetInt("ssaoInput", 0);
}

void SSAO::Update()
{
	m_GBufferSSAO.Update();
	GenerateConditional();
}

void SSAO::Render(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
	std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models)
{
	// BEGIN SSAO Rendering
	// -----------------------------------------------------------------
	GeometryPass(projectionMatrix, viewMatrix, meshes, models);
	GenerateSSAOTexture(projectionMatrix, meshes);
	BlurSSAOTexture(meshes);
	LightPass(viewMatrix, meshes);
	// -----------------------------------------------------------------
	// END SSAO Rendering
}

void SSAO::BindFramebufferSSAO()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SSAO_FBO);
}

void SSAO::BindFramebufferSSAOBlur()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SSAO_FBO_Blur);
}

void SSAO::BindColorAttachment()
{
	glReadBuffer(GL_COLOR_ATTACHMENT0);
}

void SSAO::GeometryPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
	std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models)
{
	// 1. geometry pass: render scene's geometry/color data into gbuffer
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, m_GBufferSSAO.m_GBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 projection = projectionMatrix;
	glm::mat4 view = viewMatrix;
	glm::mat4 model = glm::mat4(1.0f);
	m_ShaderGeometryPass->Bind();
	m_ShaderGeometryPass->SetMat4("projection", projection);
	m_ShaderGeometryPass->SetMat4("view", view);

	// room cube
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0, 16.0f, 0.0f));
	model = glm::scale(model, glm::vec3(16.0f, 16.0f, 16.0f));
	m_ShaderGeometryPass->SetMat4("model", model);
	m_ShaderGeometryPass->SetInt("invertedNormals", 1); // invert normals as we're inside the cube
	// RenderCube();
	meshes["cube"]->Render();
	m_ShaderGeometryPass->SetInt("invertedNormals", 0);

	// gladiator model
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	// model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.05f));
	m_ShaderGeometryPass->SetMat4("model", model);
	(*models)["gladiator"]->Draw(m_ShaderGeometryPass.Raw());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::GenerateSSAOTexture(glm::mat4 projectionMatrix, std::map<std::string, Mesh*> meshes)
{
	// 2. generate SSAO texture
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO);
	glClear(GL_COLOR_BUFFER_BIT);
	m_ShaderSSAO->Bind();

	m_ShaderSSAO->SetInt("kernelSize", m_KernelSize);
	m_ShaderSSAO->SetFloat("radius", m_KernelRadius);
	m_ShaderSSAO->SetFloat("bias", m_KernelBias);

	// Send kernel + rotation 
	for (unsigned int i = 0; i < m_KernelSize; ++i) {
		m_ShaderSSAO->SetFloat3("samples[" + std::to_string(i) + "]", m_SSAO_Kernel[i]);
	}
	m_ShaderSSAO->SetMat4("projection", projectionMatrix);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_GBufferSSAO.m_GBufferPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_GBufferSSAO.m_GBufferNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
	m_QuadSSAO->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::BlurSSAOTexture(std::map<std::string, Mesh*> meshes)
{
	if (!m_BlurEnabled) return;

	// 3. blur SSAO texture to remove noise
	// -----------------------------------------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO_Blur);
	glClear(GL_COLOR_BUFFER_BIT);
	m_ShaderSSAOBlur->Bind();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_SSAO_ColorBuffer);
	m_QuadSSAO->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAO::LightPass(glm::mat4 viewMatrix, std::map<std::string, Mesh*> meshes)
{
	// 4. lighting pass: traditional deferred Blinn-Phong lighting with added screen-space ambient occlusion
	// -----------------------------------------------------------------------------------------------------
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_ShaderLightingPass->Bind();
	// send light relevant uniforms
	glm::vec3 lightPosView = glm::vec3(viewMatrix * glm::vec4(m_LightPos, 1.0));
	m_ShaderLightingPass->SetFloat3("light.Position", lightPosView);
	m_LightColor = LightManager::directionalLight.GetColor();
	m_ShaderLightingPass->SetFloat3("light.Color", m_LightColor);
	// Update attenuation parameters
	const float linear = 0.09f;
	const float quadratic = 0.032f;
	m_ShaderLightingPass->SetFloat("light.Linear", linear);
	m_ShaderLightingPass->SetFloat("light.Quadratic", quadratic);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_GBufferSSAO.m_GBufferPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_GBufferSSAO.m_GBufferNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_GBufferSSAO.m_GBufferAlbedo);
	glActiveTexture(GL_TEXTURE3); // add extra SSAO texture to lighting pass
	glBindTexture(GL_TEXTURE_2D, m_BlurEnabled ? m_SSAO_ColorBufferBlur : m_SSAO_ColorBuffer);
	m_QuadSSAO->Render();
}
