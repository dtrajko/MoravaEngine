#pragma once

#include "RendererBasic.h"


class RendererHazelVulkan : public RendererBasic
{

public:
	RendererHazelVulkan();
	~RendererHazelVulkan();

	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;

	void RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void PostProcessing(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

};
