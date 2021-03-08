#pragma once

#include "RendererBasic.h"

#include "Deferred/GBuffer.h"


class RendererDeferredOGL : public RendererBasic
{

public:
	RendererDeferredOGL();
	~RendererDeferredOGL();

	virtual void Init(Scene* scene) override;
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);

	virtual void SetShaders() override;

	void GeometryPass(Scene* scene, glm::mat4 projectionMatrix);
	void LightPass(Window* mainWindow);

private:
	GBuffer m_gbuffer;

	Hazel::Ref<Shader> m_ShaderGeometryPass;
	Shader* m_ShaderForwardBasic;

};
