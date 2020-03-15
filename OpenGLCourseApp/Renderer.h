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

	void RenderOmniShadows(glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	void RenderWaterEffects(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, float deltaTime);

	void RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, WaterManager* waterManager);
	void RenderPassShadow(DirectionalLight* light, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	void RenderPassOmniShadow(PointLight* light, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	void RenderPassWaterReflection(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene);
	void RenderPassWaterRefraction(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene);
	~Renderer();

};
