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
		// static void DrawOld(HazelCamera* camera); // TODO: Temporary method until composite rendering is enabled

		static void ShowExampleAppDockSpace(bool* p_open); // ImGui docking
		static void UpdateImGuizmo(Window* mainWindow);

		static uint32_t GetViewportWidth();
		static uint32_t GetViewportHeight();

	};

}
