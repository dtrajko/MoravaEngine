#pragma once

#include "RendererBasic.h"

#include "learnopengl/ModelJoey.h"
#include "AABB.h"


class RendererEditor : public RendererBasic
{

public:
	RendererEditor();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererEditor();

};
