#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RendererCapabilities.h"
#include "Hazel/Renderer/SceneRenderer.h"

#include "Core/Window.h"


namespace Hazel {

	class VulkanRenderer : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		virtual void BeginRenderPass(const Ref<RenderPass>& renderPass) override;
		virtual void EndRenderPass() override;
		virtual void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material) override;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) override;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) override;

		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual RendererCapabilities& GetCapabilities() override;

		static void SubmitMeshTemp(const Ref<HazelMesh>& mesh, const glm::mat4& transform = glm::mat4(1.0f)); // to be removed from VulkanRenderer
		static void OnResize(uint32_t width, uint32_t height);                                                // to be removed from VulkanRenderer
		static uint32_t GetViewportWidth();                                                                   // to be removed from VulkanRenderer
		static uint32_t GetViewportHeight();                                                                  // to be removed from VulkanRenderer

		static void RenderMeshVulkan(Ref<HazelMesh> mesh, VkCommandBuffer commandBuffer);

		static void RenderSkybox(VkCommandBuffer commandBuffer);

		static void Draw(HazelCamera* camera); // TODO: there should be no parameters
		static void GeometryPass();
		static void CompositePass();
		static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
		static glm::vec3 GetLightDirectionTemp();

		// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);

		static int32_t& GetSelectedDrawCall();

		static void SetCamera(HazelCamera& camera);

		/**** BEGIN methods moved from VulkanTestLayer to VulkanRenderer ****/
		static SceneRendererOptions& GetOptions(); // moved from VulkanTestLayer to VulkanRenderer
		// static void MapUniformBuffersVTL(const glm::vec4& clearColor, const EditorCamera& camera);
		/**** END methods moved from VulkanTestLayer to VulkanRenderer ****/

	public:
		static bool s_MipMapsEnabled;
		static bool s_ViewportFBNeedsResize;

	};

}
