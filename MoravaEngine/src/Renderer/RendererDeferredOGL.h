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

	virtual void SetShaders() override;

	void GeometryPass();
	void LightPass();

private:
	GBuffer m_gbuffer;

	Hazel::Ref<Shader> m_ShaderGeometryPass;

};
