#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"

#include "DX11Texture2D.h"
#include "DX11ConstantBuffer.h"
#include "DX11TestLayer.h"


#include "Core/Window.h"


__declspec(align(16))
struct DX11ConstantBufferLayout
{
	glm::mat4 Model          = glm::mat4(1.0f);
	glm::mat4 View           = glm::mat4(1.0f);
	glm::mat4 Projection     = glm::mat4(1.0f);
	glm::vec4 LightDirection = glm::vec4(0.0f);
	glm::vec4 CameraPosition = glm::vec4(0.0f);
	glm::vec4 LightPosition  = glm::vec4(0.0f);
	float LightRadius        = 0;
	uint32_t Time            = 0;
};


class DX11Renderer
{
public:
	static void Init();

	static void Draw(Hazel::HazelCamera* camera); // TODO: there should be no parameters
	static void OnResize(uint32_t width, uint32_t height);

	static void SubmitMesh(RenderObject renderObject);
	static void RenderMesh(RenderObject renderObject);

	static void BeginFrame();
	static void EndFrame();

	static void BeginRenderPass(const Hazel::Ref<Hazel::RenderPass>& renderPass);
	static void EndRenderPass();
	static void SubmitFullscreenQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Material> material);

	static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
	static void UpdateImGuizmo(Window* mainWindow, Hazel::HazelCamera* camera);

	static uint32_t GetViewportWidth();
	static uint32_t GetViewportHeight();

	// DirectX 11 drawing methods (moved from DX11Context)
	static void ClearRenderTargetColor(float red, float green, float blue, float alpha);
	static void ClearRenderTargetColor(Hazel::Ref<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha);
	static void ClearDepthStencil();
	static void ClearDepthStencil(Hazel::Ref<DX11Texture2D> depthStencil);

	static void DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex);
	static void DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation);
	static void DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex);

};
