#pragma once

#include "RendererBasic.h"


class RendererTrivial : public RendererBasic
{

public:
	RendererTrivial();
	~RendererTrivial();

	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;

	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

};
