#pragma once

#include "RendererBasic.h"


class RendererAsteroids : public RendererBasic
{

public:
	RendererAsteroids();
	virtual void Init() override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererAsteroids();

private:
	unsigned int quadVAO;
	unsigned int quadVBO;

};
