#pragma once

#include "RendererBasic.h"

#include "SceneCubemaps.h"


class RendererCubemaps : public RendererBasic
{

public:
	RendererCubemaps();
	virtual void Init(Scene* scene) override;
	virtual void SetUniforms() override;
	virtual void SetShaders() override;
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void DrawLine(glm::vec3 start, glm::vec3 end, Shader* shader, glm::mat4 projectionMatrix, glm::mat4 viewMatrix);
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	~RendererCubemaps();

private:
	std::map<std::string, ModelJoey*> models;

};
