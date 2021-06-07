#pragma once

#include "Hazel/Renderer/RendererAPI.h"

#include "Core/Log.h"
#include "Core/Window.h"
#include "Scene/Scene.h"
#include "Shader/MoravaShader.h"

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

	virtual void BeginFrame() = 0;
	virtual void WaitAndRender(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	static void AppendRendererInfo(std::string& windowTitle);
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
	static void EnableWireframe();
	static void DisableWireframe();

	static void SetupViewportSize(uint32_t width, uint32_t height);

	static glm::mat4 GetProjectionMatrix();
	static void SetProjectionMatrix(glm::mat4 projectionMatrix);
	static void SetDefaultFramebuffer(unsigned int width, unsigned int height);

	static std::map<std::string, MoravaShader*>& GetShaders();
	static std::map<std::string, int>& GetUniforms();

	static glm::vec4 GetBgColor();

	static void RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow);
	static void Cleanup();
	static void Clear();
	static void Clear(float r, float g, float b, float a);

	static void SetLineThickness(float thickness); // RendererAPI::SetLineThickness

	// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
	static void DrawIndexed(uint32_t count, Hazel::PrimitiveType type, bool depthTest);

	// static void SetSpirVEnabled(bool enabled) { s_SpirV_Enabled = enabled; }
	static bool GetVulkanSupported();

protected:
	static void UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene);

};
