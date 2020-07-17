#pragma once

#include "RendererBasic.h"


class RendererInstanced : public RendererBasic
{

public:
	RendererInstanced();
	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererInstanced();

private:
	unsigned int quadVAO;
	unsigned int quadVBO;

};
