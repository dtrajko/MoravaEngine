#pragma once

#include "RendererBasic.h"


class Renderer : public RendererBasic
{

public:
	Renderer();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void BeginFrame() override;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPassMain(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderOmniShadows(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderWaterEffects(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassShadow(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterReflection(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterRefraction(Window* mainWindow, Scene* scene ,glm::mat4 projectionMatrix);
	~Renderer();

};
