#pragma once

#include "glm/glm.hpp"

#include "Window.h"
#include "Scene.h"
#include "Shader.h"


class RendererBasic
{

public:

	static void Init();
	static void SetUniforms();
	static void SetShaders();
	static std::map<std::string, Shader*> GetShaders() { return shaders; };

	static void RenderPass(glm::mat4 projectionMatrix, Window& mainWindow, Scene* scene, Camera* camera);

	static void EnableCulling();
	static void DisableCulling();

	static void Cleanup();

protected:
	static std::map<std::string, Shader*> shaders;
	static std::map<std::string, GLint> uniforms;
	static glm::vec4 bgColor;
};
