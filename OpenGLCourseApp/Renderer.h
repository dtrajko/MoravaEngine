#pragma once

#include "glm/glm.hpp"

#include "Window.h"
#include "Scene.h"
#include "Shader.h"


class Renderer
{

public:
	static void Init();
	static void SetUniforms();
	static void SetShaders();
	static std::map<std::string, Shader*> GetShaders() { return shaders; };
	static void RenderPass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera);
	static void RenderPassShadow(DirectionalLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene);
	static void RenderPassOmniShadow(PointLight* light, glm::mat4 viewMatrix, glm::mat4 projectionMatrix, Scene* scene);
	static void Cleanup();

private:
	static std::map<std::string, Shader*> shaders;
	static std::map<std::string, GLint> uniforms;

};
