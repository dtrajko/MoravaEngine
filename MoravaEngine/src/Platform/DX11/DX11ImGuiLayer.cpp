#include "DX11ImGuiLayer.h"

#include "Core/Application.h"
#include "DX11.h"
#include "DX11Context.h"
#include "DX11Device.h"
#include "DX11SwapChain.h"

#include "Hazel/Renderer/HazelRenderer.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "imgui.h"
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
	: ImGuiLayer(name)
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
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	// io.ConfigViewportsNoAutoMerge = true;
	// io.ConfigViewportsNoTaskBarIcon = true;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Bold.ttf", 16.0f);
	io.FontDefault = io.Fonts->AddFontFromFileTTF("Fonts/opensans/OpenSans-Regular.ttf", 16.0f);

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	HWND hwnd = Application::Get()->GetWindow()->GetHWND();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(DX11Context::Get()->GetDX11Device(), DX11Context::Get()->GetImmediateContext());

	// HazelRenderer::Submit([]{
	// });

	{
		// Setup Platform/Renderer bindings

		// Load Fonts
		// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
		// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
		// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
		// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
		// - Read 'docs/FONTS.md' for more instructions and details.
		// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
		// io.Fonts->AddFontDefault();
		// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
		// io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
		// io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
		// io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
		// ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
		// IM_ASSERT(font != NULL);

		// Upload Fonts
		{
			// Use any command queue
			// ImGui_ImplVulkan_CreateFontsTexture
			// ImGui_ImplVulkan_DestroyFontUploadObjects
		}
	}
}

void DX11ImGuiLayer::OnDetach()
{
	// HazelRenderer::Submit([] {});

	// ImGui Cleanup
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void DX11ImGuiLayer::Begin()
{
	ImGuiIO& io = ImGui::GetIO();

	float time = (float)Timer::Get()->GetCurrentTimestamp();
	io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
	m_Time = time;

	// Start the Dear ImGui frame
	// ImGui::Render();
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	// ImGui::NewFrame();
}

void DX11ImGuiLayer::End()
{
	ImGuiIO& io = ImGui::GetIO();
	Application* app = Application::Get();
	io.DisplaySize = ImVec2(static_cast<float>(app->GetWindow()->GetWidth()), static_cast<float>(app->GetWindow()->GetHeight()));

	// Rendering here only in OpenGL version (?)
	// ImGui::Render();
	// ImDrawData* main_draw_data = ImGui::GetDrawData();
	// ImGui_ImplDX11_RenderDrawData(main_draw_data);
}

void DX11ImGuiLayer::OnImGuiRender()
{
}
