#include "DX11ImGuiLayer.h"

#include "Core/Application.h"
#include "DX11.h"
#include "DX11Context.h"
#include "DX11Device.h"
#include "DX11SwapChain.h"

#include "H2M/Renderer/RendererH2M.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "ImGuizmo.h"

#include <tchar.h>


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

DX11ImGuiLayer::DX11ImGuiLayer()
{
	Log::GetLogger()->info("DX11ImGuiLayer created!");
}

DX11ImGuiLayer::DX11ImGuiLayer(const std::string& name)
	: ImGuiLayerH2M(name)
{
	Log::GetLogger()->info("DX11ImGuiLayer('{0}') created!", name);
}

DX11ImGuiLayer::~DX11ImGuiLayer()
{
	Log::GetLogger()->info("DX11ImGuiLayer destroyed!");

	// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();
}

void DX11ImGuiLayer::OnAttach()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Bold.ttf", 16.0f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Regular.ttf", 16.0f);

	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	HWND hwnd = Application::Get()->GetWindow()->GetHWND();

	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(
		DX11Context::Get()->GetDX11Device(),
		DX11Context::Get()->GetDX11DeviceContext()
	);
}

void DX11ImGuiLayer::OnDetach()
{
	// ImGui Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void DX11ImGuiLayer::OnUpdate(H2M::TimestepH2M ts)
{
}

void DX11ImGuiLayer::OnEvent(H2M::EventH2M& event)
{
}

void DX11ImGuiLayer::Begin()
{
	ImGuiIO& io = ImGui::GetIO();
	
	float time = (float)Timer::Get()->GetCurrentTimestamp();
	io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
	m_Time = time;
	
	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuizmo::BeginFrame();
}

void DX11ImGuiLayer::End()
{
	ImGuiIO& io = ImGui::GetIO();
	Application* app = Application::Get();
	io.DisplaySize = ImVec2(static_cast<float>(app->GetWindow()->GetWidth()), static_cast<float>(app->GetWindow()->GetHeight()));
	
	// Rendering

	// Assemble together Draw Data
	ImGui::Render();

	// Render Draw Data
	ImDrawData* main_draw_data = ImGui::GetDrawData();
	ImGui_ImplDX11_RenderDrawData(main_draw_data);

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		if (!app->GetWindow()->GetShouldClose())
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
}

void DX11ImGuiLayer::OnImGuiRender()
{
}
