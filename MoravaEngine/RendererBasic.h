#pragma once

#include "glm/glm.hpp"

#include "Window.h"
#include "Scene.h"
#include "Shader.h"


class RendererBasic
{

public:

	RendererBasic();
	virtual ~RendererBasic();

	virtual void Init(Scene* scene); // Scene param here could be a mistake
	virtual void SetUniforms();
	virtual void SetShaders();
	virtual void Render(float deltaTime, Window& mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	static void EnableCulling();
	static void DisableCulling();
	static void EnableTransparency();
	static void DisableTransparency();
	static void ClearDepthBuffer();
	static inline glm::mat4 GetProjectionMatrix() { return s_ProjectionMatrix; };
	static inline void SetProjectionMatrix(glm::mat4 projectionMatrix) { s_ProjectionMatrix = projectionMatrix; };

	std::map<std::string, Shader*> GetShaders() { return shaders; };
	void RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window& mainWindow);
	void SetDefaultFramebuffer(unsigned int width, unsigned int height);
	void Cleanup();

protected:
	static void UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene);

private:
	static glm::mat4 s_ProjectionMatrix;

protected:

	std::map<std::string, Shader*> shaders;
	std::map<std::string, GLint> uniforms;
	glm::vec4 bgColor;

};
