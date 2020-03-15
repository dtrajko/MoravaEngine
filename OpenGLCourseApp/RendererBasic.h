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
	void RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene);
	void EnableCulling();
	void DisableCulling();
	void Cleanup();
	~RendererBasic();

protected:
	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;
	glm::vec4 bgColor;
};
