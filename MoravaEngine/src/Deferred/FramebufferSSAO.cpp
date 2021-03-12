#include "FramebufferSSAO.h"

#include "Core/Log.h"
#include "Core/Math.h"


FramebufferSSAO::FramebufferSSAO()
{
}

FramebufferSSAO::~FramebufferSSAO()
{
	m_KernelSize = 64;
	m_Radius = 0.5f;
	m_Bias = 0.025f;
}

bool FramebufferSSAO::Init(unsigned int width, unsigned int height)
{
	glGenFramebuffers(1, &m_SSAO_FBO);

	// SSAO  color buffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO);
	uint32_t m_SSAO_ColorBuffer;
	glGenTextures(1, &m_SSAO_ColorBuffer);
	glBindTexture(GL_TEXTURE_2D, m_SSAO_ColorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAO_ColorBuffer, 0);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		Log::GetLogger()->error("FramebufferSSAO::Init: SSAO Framebuffer not complete!");
		return false;
	}

	m_ShaderSSAO = Hazel::Ref<Shader>::Create("Shaders/LearnOpenGL/ssao.vs", "Shaders/LearnOpenGL/ssao.fs");
	Log::GetLogger()->info("FramebufferSSAO: m_ShaderSSAO compiled [programID={0}]", m_ShaderSSAO->GetProgramID());

	m_QuadSSAO = Hazel::Ref<QuadSSAO>::Create();

	GenerateSampleKernel();
	GenerateNoiseTexture();

	return true;
}

void FramebufferSSAO::Write(const glm::mat4& projection, const GBuffer& gbuffer)
{
	// Generate SSAO texture
	glBindFramebuffer(GL_FRAMEBUFFER, m_SSAO_FBO);
	glClear(GL_COLOR_BUFFER_BIT);
	m_ShaderSSAO->Bind();
	m_ShaderSSAO->setMat4("projection", projection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer.GBUFFER_TEXTURE_TYPE_POSITION);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer.GBUFFER_TEXTURE_TYPE_NORMAL);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
	m_QuadSSAO->Render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// 2. generate SSAO texture
	// -----------------------------------------------------------------
	m_ShaderSSAO->Bind();
	m_ShaderSSAO->setInt("gPosition", 0);
	m_ShaderSSAO->setInt("gNormal", 1);
	m_ShaderSSAO->setInt("texNoise", 2);
	m_ShaderSSAO->setFloat("screenWidth", (float)m_Width);
	m_ShaderSSAO->setFloat("screenHeight", (float)m_Height);
	m_ShaderSSAO->setInt("m_KernelSize", m_KernelSize);
	m_ShaderSSAO->setFloat("radius", m_Radius);
	m_ShaderSSAO->setFloat("bias", m_Bias);
}

void FramebufferSSAO::GenerateSampleKernel()
{
	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	m_ShaderSSAO->Bind();
	for (unsigned int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / 64.0f;

		// scale samples s.t. they're more aligned to center of kernel
		scale = Math::Lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		m_SSAO_Kernel.push_back(sample);
		m_ShaderSSAO->setVec3("samples[" + std::to_string(i) + "]", sample);
	}
}

void FramebufferSSAO::GenerateNoiseTexture()
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

void FramebufferSSAO::BindForWriting()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_SSAO_FBO);
}

void FramebufferSSAO::BindForReading()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SSAO_FBO);
}

void FramebufferSSAO::SetReadBuffer()
{
	glReadBuffer(GL_COLOR_ATTACHMENT0);
}
