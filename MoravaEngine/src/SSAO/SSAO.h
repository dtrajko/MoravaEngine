#pragma once

#include "Core/CommonStructs.h"
#include "LearnOpenGL/ModelSSAO.h"
#include "Mesh/Cube.h"
#include "Mesh/QuadSSAO.h"
#include "Shader/Shader.h"
#include "SSAO/GBufferSSAO.h"


class SSAO
{
public:
	SSAO();
	~SSAO();

	void Init();
	void Update();
	void Render(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
		std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models);

	void BindFramebufferSSAO();
	void BindFramebufferSSAOBlur();
	void BindColorAttachment();

private:
	// generate methods
	void GenerateConditional();

	void GenerateSSAO_FBO();
	void GenerateSSAO_BlurFBO();
	void GenerateSampleKernel();
	void GenerateNoiseTexture();
	void GenerateLightingInfo();

	// render methods
	void GeometryPass(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
		std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models);
	void GenerateSSAOTexture(glm::mat4 projectionMatrix, std::map<std::string, Mesh*> meshes);
	void BlurSSAOTexture(std::map<std::string, Mesh*> meshes);
	void LightPass(glm::mat4 viewMatrix, std::map<std::string, Mesh*> meshes);

	void SetupShaders();
	void Generate();
	void Release();
	void ResetHandlers();

public:
	// GBufferSSAO
	GBufferSSAO m_GBufferSSAO;

	// Shaders
	Shader* m_ShaderGeometryPass;
	Shader* m_ShaderLightingPass;
	Shader* m_ShaderSSAO;
	Shader* m_ShaderSSAOBlur;

	// SSAO processing stage
	unsigned int m_SSAO_FBO;
	unsigned int m_SSAO_FBO_Blur;
	unsigned int m_SSAO_ColorBuffer;
	unsigned int m_SSAO_ColorBufferBlur;

	unsigned int m_NoiseTexture;

	glm::vec3 m_LightPos;
	glm::vec3 m_LightColor;

	unsigned int m_KernelSize;
	float m_KernelRadius;
	float m_KernelBias;

	bool m_BlurEnabled;

private:
	std::vector<glm::vec3> m_SSAO_Kernel;

	uint32_t m_Width;
	uint32_t m_Height;

	uint32_t m_WidthPrev;
	uint32_t m_HeightPrev;

};
