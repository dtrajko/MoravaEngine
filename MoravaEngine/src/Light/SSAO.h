#pragma once

#include "../../LearnOpenGL/ModelSSAO.h"
#include "../../QuadSSAO.h"

#include "Core/CommonStructs.h"
#include "Mesh/Cube.h"
#include "Shader/Shader.h"


class SSAO
{
public:
	SSAO();
	~SSAO();

	void Init();
	void Update(float timestep);
	void UpdateCooldown(float timestep);
	void Render(glm::mat4 projectionMatrix, glm::mat4 viewMatrix,
		std::map<std::string, Mesh*> meshes, std::map<std::string, ModelSSAO*>* models);

private:
	// private methods
	void SetupShaders();
	void Generate(unsigned int width, unsigned int height);
	void Release();
	void ResetHandlers();

public:
	// Shaders
	Shader* shaderGeometryPass;
	Shader* shaderLightingPass;
	Shader* shaderSSAO;
	Shader* shaderSSAOBlur;

	unsigned int gPosition;
	unsigned int gNormal;
	unsigned int gAlbedo;
	unsigned int rboDepth;

	// SSAO processing stage
	unsigned int ssaoFBO;
	unsigned int ssaoBlurFBO;
	unsigned int ssaoColorBuffer;
	unsigned int ssaoColorBufferBlur;

	unsigned int gBuffer;
	unsigned int noiseTexture;

	glm::vec3 lightPos;
	glm::vec3 lightColor;

	unsigned int kernelSize;
	float radius;
	float bias;

private:
	std::vector<glm::vec3> ssaoKernel;

	unsigned int m_WidthPrev;
	unsigned int m_HeightPrev;

	EventCooldown m_UpdateCooldown;

};
