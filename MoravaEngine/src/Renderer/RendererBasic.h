#pragma once

#include "Core/Log.h"
#include "Core/Window.h"
#include "Scene/Scene.h"
#include "Shader/Shader.h"

#include "glm/glm.hpp"


class Scene;

class RendererBasic
{

public:
	RendererBasic();
	virtual ~RendererBasic();

	virtual void Init(Scene* scene); // Scene param here could be a mistake
	virtual void SetUniforms();
	virtual void SetShaders();
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	static void InitDebug();
	static void EnableCulling();
	static void DisableCulling();
	static void EnableTransparency();
	static void DisableTransparency();
	static void EnableDepthBuffer();
	static void DisableDepthBuffer();
	static void ClearDepthBuffer();
	static void EnableDepthTest();
	static void DisableDepthTest();
	static void EnableMSAA();
	static void DisableMSAA();
	static void EnableBlend();
	static void DisableBlend();
	static inline glm::mat4 GetProjectionMatrix() { return s_ProjectionMatrix; };
	static inline void SetProjectionMatrix(glm::mat4 projectionMatrix) { s_ProjectionMatrix = projectionMatrix; };
	static void SetDefaultFramebuffer(unsigned int width, unsigned int height);

	static std::map<std::string, Shader*>& GetShaders() { return s_Shaders; };
	static std::map<std::string, int>& GetUniforms() { return s_Uniforms; };

	static void RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow);
	static void Cleanup();
	static void Clear(float r, float g, float b, float a);

	static void SetLineThickness(float thickness); // RendererAPI::SetLineThickness

	// static void SetSpirVEnabled(bool enabled) { s_SpirV_Enabled = enabled; }
	static bool GetVulkanSupported();

protected:
	static void UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene);

private:
	static glm::mat4 s_ProjectionMatrix;

public:
	static std::map<std::string, Shader*> s_Shaders;
	static std::map<std::string, int> s_Uniforms;
	static glm::vec4 s_BgColor;
	// static bool s_SpirV_Enabled;

};
