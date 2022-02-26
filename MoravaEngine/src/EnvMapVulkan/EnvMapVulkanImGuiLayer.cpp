/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "EnvMapVulkanImGuiLayer.h"

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Application.h"

#include "ImGuizmo.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan_with_textures.h"

#include <GLFW/glfw3.h>


static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

EnvMapVulkanImGuiLayer::EnvMapVulkanImGuiLayer()
{
	Log::GetLogger()->info("EnvMapVulkanImGuiLayer created!");
}

EnvMapVulkanImGuiLayer::EnvMapVulkanImGuiLayer(const std::string& name)
{
	Log::GetLogger()->info("EnvMapVulkanImGuiLayer('{0}') created!", name);
}

EnvMapVulkanImGuiLayer::~EnvMapVulkanImGuiLayer()
{
	Log::GetLogger()->info("EnvMapVulkanImGuiLayer destroyed!");

	auto device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

	// Cleanup
	VkResult err = vkDeviceWaitIdle(device);
	check_vk_result(err);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	// ImGui_ImplVulkanH_DestroyWindow(VulkanContextH2M::GetInstance(), device, &g_MainWindowData, nullptr);
	vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
}

void EnvMapVulkanImGuiLayer::OnAttach()
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

		auto vulkanContext = H2M::VulkanContextH2M::Get();
		auto device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		auto currentDevice = H2M::VulkanContextH2M::GetCurrentDevice();

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};

			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			VkResult err = vkCreateDescriptorPool(device, &pool_info, nullptr, &m_DescriptorPool);
			check_vk_result(err);
		}

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = H2M::VulkanContextH2M::GetInstance();
		init_info.PhysicalDevice = currentDevice->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		init_info.Device = currentDevice->GetVulkanDevice();
		init_info.QueueFamily = currentDevice->GetPhysicalDevice()->GetQueueFamilyIndices().Graphics;
		init_info.Queue = currentDevice->GetGraphicsQueue();
		init_info.PipelineCache = nullptr;
		init_info.DescriptorPool = m_DescriptorPool;
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2; // vulkanContext->GetSwapChain().GetImageCount();
		init_info.ImageCount = app->GetWindow()->GetSwapChain().GetImageCount();
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, app->GetWindow()->GetSwapChain().GetRenderPass());

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

			m_CommandBuffer = vulkanContext->GetCurrentDevice()->GetCommandBuffer(true);
			ImGui_ImplVulkan_CreateFontsTexture(m_CommandBuffer);
			vulkanContext->GetCurrentDevice()->FlushCommandBuffer(m_CommandBuffer);

			VkResult err = vkDeviceWaitIdle(device);
			check_vk_result(err);
			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}
}

void EnvMapVulkanImGuiLayer::OnDetach()
{
	// HazelRenderer::Submit([] {
	// });
	{
		auto device = H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		auto err = vkDeviceWaitIdle(device);
		check_vk_result(err);

		// ImGui Cleanup
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
}

void EnvMapVulkanImGuiLayer::Begin()
{
	ImGuiIO& io = ImGui::GetIO();

	float time = (float)glfwGetTime();
	io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
	m_Time = time;

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	// ImGuizmo::BeginFrame();

	// ImGui::ShowDemoWindow();
}

void EnvMapVulkanImGuiLayer::End()
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
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void EnvMapVulkanImGuiLayer::OnUpdate(H2M::TimestepH2M ts)
{
}

void EnvMapVulkanImGuiLayer::OnEvent(H2M::EventH2M& event)
{
}

void EnvMapVulkanImGuiLayer::OnRender()
{
}

void EnvMapVulkanImGuiLayer::OnImGuiRender()
{
}
