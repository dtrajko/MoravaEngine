#pragma once

#include "RendererBasic.h"

#include "../../LearnOpenGL/ModelJoey.h"
#include "../../SceneNanosuit.h"


class RendererNanosuit : public RendererBasic
{

public:
	RendererNanosuit();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderPass(Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererNanosuit();

	std::map<std::string, ModelJoey*> models;
	float m_ModelRotationY = 0.0f;

	NanosuitUniforms* nanosuitUniforms;

};
