#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"

#include "DX11Texture2D.h"
#include "DX11ConstantBuffer.h"

#include "Core/Window.h"


struct DX11VertexLayout
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Binormal;
};

__declspec(align(16))
struct DX11ConstantBufferLayout
{
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Projection;
	uint32_t Time;
};


class DX11Renderer
{
public:
	static void Init();

	static void Draw(Hazel::HazelCamera* camera); // TODO: there should be no parameters
	static void OnResize(uint32_t width, uint32_t height);

	static void SubmitMesh(const Hazel::Ref<Hazel::HazelMesh>& mesh);

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

	static glm::mat4 CalculateViewMatrix();

};
