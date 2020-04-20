#pragma once

#include "glm/glm.hpp"

#include "Window.h"
#include "Scene.h"
#include "Shader.h"


class RendererBasic
{

public:

	RendererBasic();

	virtual void Init(Scene* scene); // Scene param here could be a mistake
	virtual void SetUniforms();
	virtual void SetShaders();
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window& mainWindow);
	void SetDefaultFramebuffer(unsigned int width, unsigned int height);
	void EnableCulling();
	void DisableCulling();
	void Cleanup();
	virtual ~RendererBasic();

protected:
	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;
	glm::vec4 bgColor;
};
