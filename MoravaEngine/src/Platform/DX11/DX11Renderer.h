#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Renderer/HazelMesh.h"
#include "Hazel/Renderer/HazelCamera.h"

#include "Core/Window.h"


class DX11Renderer
{
public:
	static void SubmitMesh(const Hazel::Ref<Hazel::HazelMesh>& mesh);

	static void OnResize(uint32_t width, uint32_t height);
	static void Init();
	static void Draw(Hazel::HazelCamera* camera); // TODO: there should be no parameters

	static void BeginFrame();
	static void EndFrame();

	static void BeginRenderPass(const Hazel::Ref<Hazel::RenderPass>& renderPass);
	static void EndRenderPass();
	static void SubmitFullscreenQuad(Hazel::Ref<Hazel::Pipeline> pipeline, Hazel::Ref<Material> material);

	static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
	static void UpdateImGuizmo(Window* mainWindow, Hazel::HazelCamera* camera);

	static uint32_t GetViewportWidth();
	static uint32_t GetViewportHeight();

};
