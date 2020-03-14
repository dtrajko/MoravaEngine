#pragma once

#include "RendererBasic.h"

#include "WaterManager.h"


class Renderer : public RendererBasic
{

public:

	static void Init();
	static void SetUniforms();
	static void SetShaders();
	static std::map<std::string, Shader*> GetShaders() { return shaders; };

	static void RenderOmniShadows(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	static void RenderWaterEffects(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera, float deltaTime);

	static void RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera, WaterManager* waterManager);
	static void RenderPassShadow(DirectionalLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	static void RenderPassOmniShadow(PointLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene, WaterManager* waterManager);
	static void RenderPassWaterReflection(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera);
	static void RenderPassWaterRefraction(WaterManager* waterManager, glm::mat4 projectionMatrix, Scene* scene, Camera* camera);

};
