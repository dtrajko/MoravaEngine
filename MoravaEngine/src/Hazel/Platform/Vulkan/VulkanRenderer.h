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

		virtual void SetSceneEnvironment(Ref<Environment> environment, Ref<HazelImage2D> shadow) override;
		virtual std::pair<Ref<HazelTextureCube>, Ref<HazelTextureCube>> CreateEnvironmentMap(const std::string& filepath) override;

		virtual void RenderMesh(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderMeshWithoutMaterial(Ref<Pipeline> pipeline, Ref<HazelMesh> mesh, const glm::mat4& transform) override;
		virtual void RenderQuad(Ref<Pipeline> pipeline, Ref<HazelMaterial> material, const glm::mat4& transform) override;

		virtual RendererCapabilities& GetCapabilities() override;

		static void Draw(HazelCamera* camera); // TODO: there should be no parameters

		static void SubmitMesh(const Ref<HazelMesh>& mesh);

		static void OnResize(uint32_t width, uint32_t height);

		static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow, HazelCamera* camera);

		static uint32_t GetViewportWidth();
		static uint32_t GetViewportHeight();

		static int32_t& GetSelectedDrawCall();

	};

}
