#include "VulkanImGuiLayer.h"

// ImGui includes
#if !defined(IMGUI_IMPL_API)
	#define IMGUI_IMPL_API
#endif
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan_with_textures.h"

#include "ImGuizmo.h"

#include "Core/Application.h"

#include <GLFW/glfw3.h>

#include "Hazel/Renderer/HazelRenderer.h"
#include "Vulkan.h"


namespace Hazel {

	VulkanImGuiLayer::VulkanImGuiLayer()
	{
		Log::GetLogger()->info("VulkanImGuiLayer created!");
	}

	VulkanImGuiLayer::VulkanImGuiLayer(const std::string& name)
	{
		Log::GetLogger()->info("VulkanImGuiLayer('{0}') created!", name);
	}

	VulkanImGuiLayer::~VulkanImGuiLayer()
	{
		Log::GetLogger()->info("VulkanImGuiLayer destroyed!");
	}

	void VulkanImGuiLayer::OnAttach()
	{
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

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

		//1: create descriptor pool for IMGUI
		// the size of the pool is very oversize, but it's copied from imgui demo itself.
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
		pool_info.maxSets = 1000;
		// pool_info.poolSizeCount = std::size(static_cast<uint32_t>(pool_sizes));
		pool_info.pPoolSizes = pool_sizes;

		// VkDescriptorPool imguiPool;
		// VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));

		//this initializes imgui for Vulkan
		// ImGui_ImplVulkan_InitInfo init_info = {};
		// init_info.Instance = _instance;
		// init_info.PhysicalDevice = _chosenGPU;
		// init_info.Device = _device;
		// init_info.Queue = _graphicsQueue;
		// init_info.DescriptorPool = imguiPool;
		// init_info.MinImageCount = 3;
		// init_info.ImageCount = 3;

		// ImGui_ImplVulkan_Init(&init_info, _renderPass);

		// execute a gpu command to upload imgui font textures
		//	immediate_submit([&](VkCommandBuffer cmd) {
		//		ImGui_ImplVulkan_CreateFontsTexture(cmd);
		//	});

		// clear font textures from cpu data
		ImGui_ImplVulkan_DestroyFontUploadObjects();

		//add the destroy the imgui created structures
		//	_mainDeletionQueue.push_function([=]() {
		//	
		//		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		//		ImGui_ImplVulkan_Shutdown();
		//	});

		// ----------------------------

		// ImGui_ImplGlfw_InitForVulkan(window->GetHandle(), true);
	}

	void VulkanImGuiLayer::OnDetach()
	{
	}

	void VulkanImGuiLayer::Begin()
	{
		ImGuiIO& io = ImGui::GetIO();

		float time = (float)glfwGetTime();
		io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
		m_Time = time;

		// ImGui Start the Dear ImGui frame

		ImGui_ImplVulkan_NewFrame();
	}

	void VulkanImGuiLayer::End()
	{
		// ImGui Rendering
		ImGuiIO& io = ImGui::GetIO();
		// io.DisplaySize = ImVec2((float)s_Window->GetWidth(), (float)s_Window->GetHeight());

		// Rendering

		// TODO
	}

	void VulkanImGuiLayer::OnImGuiRender()
	{
	}

}
