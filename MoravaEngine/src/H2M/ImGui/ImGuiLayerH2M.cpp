#include "ImGuiLayerH2M.h"

#include "H2M/Renderer/RendererAPI_H2M.h"

#include "H2M/Platform/OpenGL/OpenGLImGuiLayerH2M.h"
#include "H2M/Platform/Vulkan/VulkanImGuiLayerH2M.h"
#include "Platform/DX11/DX11ImGuiLayer.h"

#include <imgui.h>


namespace H2M
{

	ImGuiLayerH2M::ImGuiLayerH2M()
		: LayerH2M("Untitled Layer")
	{
	}

	ImGuiLayerH2M::ImGuiLayerH2M(const std::string& name)
		: LayerH2M(name)
	{
	}

	ImGuiLayerH2M::~ImGuiLayerH2M()
	{
	}

	ImGuiLayerH2M* ImGuiLayerH2M::Create()
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return nullptr;
		case RendererAPITypeH2M::OpenGL: return new OpenGLImGuiLayerH2M("OpenGLImGuiLayerH2M");
		case RendererAPITypeH2M::Vulkan: return new VulkanImGuiLayerH2M("VulkanImGuiLayerH2M");
		case RendererAPITypeH2M::DX11:   return new DX11ImGuiLayer("DX11ImGuiLayer");
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	void ImGuiLayerH2M::SetDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header]        = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
		colors[ImGuiCol_HeaderActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button]        = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
		colors[ImGuiCol_ButtonActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg]        = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f,  0.305f,  0.31f,  1.0f };
		colors[ImGuiCol_FrameBgActive]  = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab]                = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered]         = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive]          = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused]       = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f,  0.205f,  0.21f,  1.0f };

		// Title
		colors[ImGuiCol_TitleBg]          = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive]    = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Resize Grip
		colors[ImGuiCol_ResizeGrip]        = ImVec4(0.91f, 0.91f, 0.91f, 0.25f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.81f, 0.81f, 0.81f, 0.67f);
		colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.46f, 0.46f, 0.46f, 0.95f);

		// Scrollbar
		colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		// Check Mark
		colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

		// Slider
		colors[ImGuiCol_SliderGrab]       = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
	}

}
