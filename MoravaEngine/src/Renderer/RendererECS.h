#pragma once

#include "RendererBasic.h"


class RendererECS : public RendererBasic
{

public:
	RendererECS();
	~RendererECS();

	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

};
