#pragma once

#include "RendererBasic.h"
#include "SSAO.h"


class RendererSSAO : public RendererBasic
{

public:
	RendererSSAO();
	~RendererSSAO();

	virtual void Init(Scene* scene) override;
	virtual void SetShaders() override;
	void SetupSSAO();
	void RenderOmniShadows(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);

private:
	SSAO* m_SSAO;
};
