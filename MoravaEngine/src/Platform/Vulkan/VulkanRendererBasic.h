#pragma once

#include "Core/Log.h"
#include "Core/Window.h"
#include "Scene/Scene.h"
#include "Shader/MoravaShader.h"

#include "glm/glm.hpp"


class Scene;

class VulkanRendererBasic : public RendererBasic
{

public:
	VulkanRendererBasic();
	virtual ~VulkanRendererBasic();

	virtual void Init(Scene* scene); // Scene param here could be a mistake
	virtual void SetUniforms();
	virtual void SetShaders();
	virtual void Render(float deltaTime, Window* mainWindow, Scene* scene, glm::mat4 projectionMatrix) = 0;

	static void RendererInfo(WindowProps& windowProps);
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

	static void SetViewportSize(uint32_t width, uint32_t height);

	static void SetDefaultFramebuffer(unsigned int width, unsigned int height);

	static void RenderPassMain(Scene* scene, glm::mat4 projectionMatrix, Window* mainWindow);
	static void Clear();
	static void Clear(float r, float g, float b, float a);

	static void SetLineThickness(float thickness); // RendererAPI::SetLineThickness

	static void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, uint32_t baseVertexLocation, void* indicesPtr = nullptr);

	// Obsolete method in vulkan branch 237c6703 (OpenGL-specific)
	static void DrawIndexed(uint32_t count, H2M::PrimitiveType type, bool depthTest);

	static void SetPolygonMode(PolygonMode polygonMode);

protected:
	static void UpdateProjectionMatrix(glm::mat4* projectionMatrix, Scene* scene);

};
