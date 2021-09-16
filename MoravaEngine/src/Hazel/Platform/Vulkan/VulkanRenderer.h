#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RendererCapabilities.h"

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

		static uint32_t GetViewportWidth();
		static uint32_t GetViewportHeight();

		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) override;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) override;

		static void RenderMeshVulkan(Ref<HazelMesh> mesh, VkCommandBuffer commandBuffer);

		static void RenderSkybox(VkCommandBuffer commandBuffer);

		virtual RendererCapabilities& GetCapabilities() override;

		static void Draw(HazelCamera* camera); // TODO: there should be no parameters
		static void CompositePass();
		static void GeometryPass();
		static void OnImGuiRender(VkCommandBufferInheritanceInfo& inheritanceInfo, std::vector<VkCommandBuffer>& commandBuffers);
		static glm::vec3 GetLightDirectionTemp();

		static void SubmitMesh(const Ref<HazelMesh>& mesh);

		static void OnResize(uint32_t width, uint32_t height);

		// static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);

		static int32_t& GetSelectedDrawCall();

		// static versions (copies) of corresponding virtual methods
		static void BeginFrameStatic();
		static void BeginRenderPassStatic(const Ref<RenderPass>& renderPass);
		static void EndRenderPassStatic();
		static void SubmitFullscreenQuadStatic(Ref<Pipeline> pipeline, Ref<HazelMaterial> material);
		static void RenderMeshStatic(/*Ref<Pipeline> pipeline,*/Ref<HazelMesh> mesh, const glm::mat4& transform);
		static void RenderQuadStatic(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform);

	public:
		static bool s_MipMapsEnabled;
		static bool s_ViewportFBNeedsResize;

	};

}
