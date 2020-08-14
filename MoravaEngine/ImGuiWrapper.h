#pragma once

// ImGui includes
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "CommonValues.h"
#include "Window.h"


class ImGuiWrapper
{
public:
	static void Init(Window* window);
	static void Begin();
	static void End();
	static void Cleanup();

private:
	static Window* m_Window;
	static float m_Time;
};
