#pragma once

#include "glm/glm.hpp"

#include "Window.h"
#include "Scene.h"
#include "Shader.h"


class RendererBasic
{

public:

	RendererBasic();
	void Init();
	void SetUniforms();
	void SetShaders();
	std::map<std::string, Shader*> GetShaders() { return shaders; };
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;
	void RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window& mainWindow);
	void EnableCulling();
	void DisableCulling();
	void Cleanup();
	~RendererBasic();

protected:
	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;
	glm::vec4 bgColor;
};
