#include "DX11ImGuiLayer.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
// #include "backends/imgui_impl_vulkan_with_textures.h"

#include "ImGuizmo.h"

#include "Core/Application.h"

#include <GLFW/glfw3.h>

#include "DX11.h"
#include "Hazel/Renderer/HazelRenderer.h"
#include "Platform/DX11/DX11Context.h"
#include "Platform/DX11/DX11Device.h"
// #include "Platform/DX11/DX11SwapChain.h"


DX11ImGuiLayer::DX11ImGuiLayer()
{
	Log::GetLogger()->info("DX11ImGuiLayer created!");
}

DX11ImGuiLayer::DX11ImGuiLayer(const std::string& name)
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

	// HazelRenderer::Submit([]{
	// });

	{
		Application* app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app->GetWindow()->GetHandle());

		auto DX11Context = DX11Context::Get();
		// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();

		auto currentDevice = DX11Context::GetCurrentDevice();

		// Setup Platform/Renderer bindings

		// Init GLFW for DirectX 11

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
	// HazelRenderer::Submit([] {
	// });
	{
		// auto device = DX11Context::GetCurrentDevice()->GetDX11Device();

		// ImGui Cleanup
		// ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

void DX11ImGuiLayer::Begin()
{
	ImGuiIO& io = ImGui::GetIO();

	float time = (float)glfwGetTime();
	io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
	m_Time = time;

	// Start the Dear ImGui frame
	// TODO: ImGui_ImplVulkan_NewFrame();
	// ImGui_ImplGlfw_NewFrame();
	// TODO: ImGui::NewFrame();
	// TODO: ImGuizmo::BeginFrame();
	// TODO: ImGui::ShowDemoWindow();
}

void DX11ImGuiLayer::End()
{
	ImGuiIO& io = ImGui::GetIO();
	Application* app = Application::Get();
	io.DisplaySize = ImVec2(static_cast<float>(app->GetWindow()->GetWidth()), static_cast<float>(app->GetWindow()->GetHeight()));

	// Rendering here only in OpenGL version (?)
	// ImGui::Render();
	// ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffer);

	// Update and Render additional Platform Windows
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		// TODO: ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void DX11ImGuiLayer::OnImGuiRender()
{
}
