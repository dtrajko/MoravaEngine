#pragma once

#include "RendererBasic.h"

#include "Framebuffer.h"


class RendererSSAO : public RendererBasic
{

public:
	RendererSSAO();
	~RendererSSAO();

	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;
	void SetupSSAO();
	void RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	float Lerp(float a, float b, float f);
	void RenderQuad();
	void RenderCube();

private:
	uint32_t gPosition;
	uint32_t gNormal;
	uint32_t gAlbedo;
	uint32_t rboDepth;

	// SSAO processing stage
	uint32_t ssaoFBO;
	uint32_t ssaoBlurFBO;
	uint32_t ssaoColorBuffer;
	uint32_t ssaoColorBufferBlur;

	uint32_t gBuffer;
	uint32_t noiseTexture;

	glm::vec3 lightPos;
	glm::vec3 lightColor;

	std::vector<glm::vec3> ssaoKernel;

	uint32_t quadVAO = 0;
	uint32_t quadVBO = 0;

	uint32_t cubeVAO = 0;
	uint32_t cubeVBO = 0;
};
