#pragma once

#include "RendererBasic.h"


class RendererECS : public RendererBasic
{

public:
	RendererECS();
	~RendererECS();

	virtual void Init(Scene* scene) override;

	virtual void BeginFrame() override;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

	virtual void SetShaders() override;

	void RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void PostProcessing(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);


};
