#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"

#include "Core/Window.h"


namespace Hazel {

	class VulkanRenderer
	{
	public:
		static void SubmitMesh(const Ref<HazelMesh>& mesh);

		static void OnResize(uint32_t width, uint32_t height);
		static void Init();
		static void Draw(HazelCamera* camera); // TODO: there should be no parameters

		static void BeginFrame();
		static void EndFrame();

		static void BeginRenderPass(const Ref<RenderPass>& renderPass);
		static void EndRenderPass();
		static void SubmitFullscreenQuad(Ref<Pipeline> pipeline, Ref<Material> material);

		static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow, HazelCamera* camera);

		static uint32_t GetViewportWidth();
		static uint32_t GetViewportHeight();

		static int32_t& GetSelectedDrawCall();

	};

}
