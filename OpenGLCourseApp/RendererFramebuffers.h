#pragma once

#include "RendererBasic.h"

#include "SceneFramebuffers.h"
#include "Framebuffer.h"


class RendererFramebuffers : public RendererBasic
{

public:
	RendererFramebuffers();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererFramebuffers();

private:
	unsigned int SCR_WIDTH = 1280;
	unsigned int SCR_HEIGHT = 720;

	// unsigned int m_FBO;
	// unsigned int textureColorbuffer;
	// unsigned int m_RBO;

	// OOP
	Framebuffer* m_Framebuffer;

	unsigned int cubeVAO;
	unsigned int cubeVBO;

	unsigned int planeVAO;
	unsigned int planeVBO;

	unsigned int quadVAO;
	unsigned int quadVBO;

};
