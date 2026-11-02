#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "RendererBasic.h"


class RendererTrivial : public RendererBasic
{

public:
	RendererTrivial();
	~RendererTrivial();

	virtual void Init(Scene* scene) override;

	virtual void BeginFrame() override;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;

	virtual void SetShaders() override;

	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

};
