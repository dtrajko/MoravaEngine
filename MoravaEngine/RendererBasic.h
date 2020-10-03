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
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	static void EnableCulling();
	static void DisableCulling();
	static void EnableTransparency();
	static void DisableTransparency();
	static void EnableDepthBuffer();
	static void DisableDepthBuffer();
	static void ClearDepthBuffer();
	static inline glm::mat4 GetProjectionMatrix() { return s_ProjectionMatrix; };
	static inline void SetProjectionMatrix(glm::mat4 projectionMatrix) { s_ProjectionMatrix = projectionMatrix; };
	static void SetDefaultFramebuffer(unsigned int width, unsigned int height);

	static std::map<std::string, Shader*>& GetShaders() { return s_Shaders; };
	static std::map<std::string, int>& GetUniforms() { return s_Uniforms; };

	static void RenderPass(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow);
	static void Cleanup();
	static void Clear(float r, float g, float b, float a);

protected:
	static void UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene);

private:
	static glm::mat4 s_ProjectionMatrix;

public:
	static std::map<std::string, Shader*> s_Shaders;
	static std::map<std::string, int> s_Uniforms;
	static glm::vec4 s_BgColor;

};
