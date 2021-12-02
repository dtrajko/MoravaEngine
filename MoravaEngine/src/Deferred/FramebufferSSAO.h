#pragma once

#include "H2M/Core/RefH2M.h"

#include "Shader/MoravaShader.h"
#include "Deferred/GBuffer.h"
#include "Mesh/QuadSSAO.h"

#include <GL/glew.h>

#include <cstdint>
#include <random>


class FramebufferSSAO
{
public:
	FramebufferSSAO();
	~FramebufferSSAO();

	bool Init(unsigned int width, unsigned int height);
	void Write(const glm::mat4& projection, const GBuffer& gbuffer);
	void BindForWriting();
	void BindForReading();
	void SetReadBuffer();

	inline uint32_t GetWidth() { return m_Width; }
	inline uint32_t GetHeight() { return m_Height; }

private:
	void GenerateSampleKernel();
	void GenerateNoiseTexture();

private:
	uint32_t m_SSAO_FBO;
	uint32_t m_SSAO_ColorBuffer;

	uint32_t m_NoiseTexture;

	H2M::RefH2M<MoravaShader> m_ShaderSSAO;

	H2M::RefH2M<QuadSSAO> m_QuadSSAO;

	std::vector<glm::vec3> m_SSAO_Kernel;

	unsigned int m_KernelSize;
	float m_Radius;
	float m_Bias;

	uint32_t m_Width;
	uint32_t m_Height;

	uint32_t m_WidthPrev;
	uint32_t m_HeightPrev;

};
