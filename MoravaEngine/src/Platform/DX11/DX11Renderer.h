#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "DX11Texture2D.h"
#include "DX11ConstantBuffer.h"
#include "DX11TestLayer.h"


#include "Core/Window.h"
#include "Framebuffer/MoravaFramebuffer.h"


__declspec(align(16))
struct DX11ConstantBufferLayout
{
	glm::mat4 Model          = glm::mat4(1.0f);
	glm::mat4 View           = glm::mat4(1.0f);
	glm::mat4 Projection     = glm::mat4(1.0f);
	glm::vec3 LightDirection = glm::vec3(0.0f);
	glm::vec3 CameraPosition = glm::vec3(0.0f);
	glm::vec3 LightPosition  = glm::vec3(0.0f);
	float LightRadius        = 0;
	uint32_t Time            = 0;
};


class DX11Renderer : public H2M::RendererAPI_H2M
{
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	virtual void BeginFrame() override;
	virtual void EndFrame() override;

	virtual void BeginRenderPass(const H2M::RefH2M<H2M::RenderPassH2M>& renderPass) override;
	virtual void EndRenderPass() override;
	virtual void SubmitFullscreenQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material) override;

	virtual void SetSceneEnvironment(H2M::RefH2M<H2M::EnvironmentH2M> environment, H2M::RefH2M<H2M::Image2D_H2M> shadow) override;

	virtual void RenderMesh(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform) override;
	virtual void RenderMeshWithoutMaterial(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MeshH2M> mesh, const glm::mat4& transform) override;
	virtual void RenderQuad(H2M::RefH2M<H2M::PipelineH2M> pipeline, H2M::RefH2M<H2M::MaterialH2M> material, const glm::mat4& transform) override;

	virtual std::pair<H2M::RefH2M<H2M::TextureCubeH2M>, H2M::RefH2M<H2M::TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

	virtual H2M::RendererCapabilitiesH2M& GetCapabilities() override;

	static void Update();

	static void Draw(H2M::CameraH2M* camera); // TODO: there should be no parameters
	static void OnResize(uint32_t width, uint32_t height);

	static void SubmitMesh(RenderObject renderObject);
	static void RenderMesh(RenderObject renderObject);
	static void RenderMeshDX11(RenderObject renderObject, const std::vector<H2M::RefH2M<DX11Material>>& listMaterials);
	static void RenderMeshesECS(); // the code is taken from EnvMapSceneRenderer::GeometryPass()

	static void RenderImGui();
	static void UpdateImGuizmo();
	static void DrawEntityNode(const std::string name); // similar to SceneHierarchyPanel::DrawEntityNode
	static void DrawComponent(const std::string name); // similar to DrawComponent in SceneHierarchyPanel
	static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
	// from EnvMapEditorLayer, previously in SceneHazelEnvMap
	static void SetupRenderFramebuffer();
	static void ResizeViewport(glm::vec2 viewportPanelSize, H2M::RefH2M<MoravaFramebuffer> renderFramebuffer);
	static void DisplaySubmeshMaterialSelector(bool* p_open);

	static void SelectEntity(H2M::EntityH2M e);
	static void OnEntityDeleted(H2M::EntityH2M e);

	static uint32_t GetViewportWidth();
	static uint32_t GetViewportHeight();

	// DirectX 11 drawing methods (moved from DX11Context)
	static void ClearRenderTargetColorSwapChain(float red, float green, float blue, float alpha);
	static void ClearRenderTargetColor(H2M::RefH2M<DX11Texture2D> renderTarget, float red, float green, float blue, float alpha);
	static void ClearDepthStencilSwapChain();
	static void ClearDepthStencil(H2M::RefH2M<DX11Texture2D> depthStencil);

	// this should probably be DX11Framebuffer->Bind()
	static void SetRenderTarget(H2M::RefH2M<DX11Texture2D> renderTarget, H2M::RefH2M<DX11Texture2D> depthStencil);

	static void DrawTriangleList(uint32_t vertexCount, uint32_t startVertexIndex);
	static void DrawIndexedTriangleList(uint32_t indexCount, uint32_t startVertexIndex, uint32_t startIndexLocation);
	static void DrawTriangleStrip(uint32_t vertexCount, uint32_t startVertexIndex);

	// we split DX11Renderer::Draw() to 2 stages
	static void DrawToFramebuffer(H2M::CameraH2M* camera);
	static void DrawToScreen(H2M::CameraH2M* camera);

	// helper methods
	static void CreateCube();
	static void CreateQuad();

};
