#pragma once

#include "RendererBasic.h"

#include "WaterManager.h"


class Renderer : public RendererBasic
{

public:

	Renderer();
	void Init();
	void SetUniforms();
	void SetShaders();
	std::map<std::string, Shader*> GetShaders() { return shaders; };

	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) override;
	void RenderOmniShadows(Scene* scene, glm::mat4 projectionMatrix);
	void RenderWaterEffects(float deltaTime, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPass(Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassShadow(Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassOmniShadow(PointLight* light, Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterReflection(Scene* scene, glm::mat4 projectionMatrix);
	void RenderPassWaterRefraction(Scene* scene ,glm::mat4 projectionMatrix);
	~Renderer();

};
