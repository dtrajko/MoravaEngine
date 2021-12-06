/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Renderer/MeshH2M.h"
#include "H2M/Renderer/CameraH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "H2M/Renderer/RendererCapabilitiesH2M.h"
#include "H2M/Renderer/SceneRendererH2M.h"

#include "Core/Window.h"


namespace H2M {

	class VulkanRendererH2M : public RendererAPI_H2M
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(const RefH2M<RenderPassH2M>& renderPass) override;
		virtual void EndRenderPass() override;
		virtual void SubmitFullscreenQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material) override;

		virtual void SetSceneEnvironment(RefH2M<EnvironmentH2M> environment, RefH2M<Image2D_H2M> shadow) override;

		virtual void RenderMesh(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(RefH2M<PipelineH2M> pipeline, RefH2M<MeshH2M> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(RefH2M<PipelineH2M> pipeline, RefH2M<MaterialH2M> material, const glm::mat4& transform) override;

		virtual std::pair<RefH2M<TextureCubeH2M>, RefH2M<TextureCubeH2M>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual RendererCapabilitiesH2M& GetCapabilities() override;

		static void SubmitMeshTemp(const RefH2M<MeshH2M>& mesh, const glm::mat4& transform = glm::mat4(1.0f)); // to be removed from VulkanRenderer
		static void OnResize(uint32_t width, uint32_t height);                                                // to be removed from VulkanRenderer
		static uint32_t GetViewportWidth();                                                                   // to be removed from VulkanRenderer
		static uint32_t GetViewportHeight();                                                                  // to be removed from VulkanRenderer

		static void RenderMeshVulkan(RefH2M<MeshH2M> mesh, VkCommandBuffer commandBuffer);

		static void RenderSkybox(VkCommandBuffer commandBuffer);

		static void Draw(CameraH2M* camera); // TODO: there should be no parameters
		static void GeometryPass();
		static void CompositePass();
		static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
		static glm::vec3 GetLightDirectionTemp();

		// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);

		static int32_t& GetSelectedDrawCall();

		static void SetCamera(CameraH2M& camera);

		/**** BEGIN methods moved from VulkanTestLayer to VulkanRenderer ****/
		static SceneRendererOptionsH2M& GetOptions(); // moved from VulkanTestLayer to VulkanRenderer
		static void MapUniformBuffersVTL(RefH2M<MeshH2M> mesh, const EditorCameraH2M& camera);
		/**** END methods moved from VulkanTestLayer to VulkanRenderer ****/

	public:
		static bool s_MipMapsEnabled;
		static bool s_ViewportFBNeedsResize;

	};

}
