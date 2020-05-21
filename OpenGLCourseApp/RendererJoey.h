#pragma once

#include "RendererBasic.h"


class RendererJoey : public RendererBasic
{

public:
	RendererJoey();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass();
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	virtual ~RendererJoey() override;

};
