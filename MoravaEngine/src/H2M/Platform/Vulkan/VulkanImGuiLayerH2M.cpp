/**
 * @package H2M
 * @author Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanImGuiLayerH2M.h"

#include "VulkanH2M.h"
#include "H2M/Renderer/RendererH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

#include "Core/Application.h"

#include "ImGuizmo.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>


namespace H2M
{

	namespace
	{
		static void check_vk_result(VkResult err)
		{
			if (err == VK_SUCCESS)
				return;

			fprintf(stderr, "[vulkan] Error: VkResult = %d\n", (int)err);

			if (err < 0)
				abort();
		}
	}


	VulkanImGuiLayerH2M::VulkanImGuiLayerH2M()
	{
		Log::GetLogger()->info("VulkanImGuiLayerH2M created!");
	}


	VulkanImGuiLayerH2M::VulkanImGuiLayerH2M(const std::string& name)
	{
		Log::GetLogger()->info("VulkanImGuiLayerH2M('{0}') created!", name);
	}


	VulkanImGuiLayerH2M::~VulkanImGuiLayerH2M()
	{
		Log::GetLogger()->info("VulkanImGuiLayerH2M destroyed!");

		// Cleanup is handled in OnDetach()
	}


	void VulkanImGuiLayerH2M::OnAttach()
	{
		IMGUI_CHECKVERSION();

		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;


		Application* app = Application::Get();

		GLFWwindow* window =
			static_cast<GLFWwindow*>(app->GetWindow()->GetHandle());


		// GLFW Vulkan backend
		ImGui_ImplGlfw_InitForVulkan(
			window,
			true
		);


		io.Fonts->AddFontFromFileTTF(
			"Fonts/opensans/OpenSans-Bold.ttf",
			16.0f
		);

		io.FontDefault =
			io.Fonts->AddFontFromFileTTF(
				"Fonts/opensans/OpenSans-Regular.ttf",
				16.0f
			);


		ImGui::StyleColorsDark();


		ImGuiStyle& style = ImGui::GetStyle();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}



		auto context = VulkanContextH2M::Get();

		auto device =
			VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();

		auto currentDevice =
			VulkanContextH2M::GetCurrentDevice();



		// Descriptor pool
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


			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType =
				VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

			poolInfo.flags =
				VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			poolInfo.maxSets =
				1000 * IM_ARRAYSIZE(pool_sizes);

			poolInfo.poolSizeCount =
				(uint32_t)IM_ARRAYSIZE(pool_sizes);

			poolInfo.pPoolSizes =
				pool_sizes;


			check_vk_result(
				vkCreateDescriptorPool(
					device,
					&poolInfo,
					nullptr,
					&m_DescriptorPool
				)
			);
		}



		ImGui_ImplVulkan_InitInfo initInfo{};

		initInfo.Instance =
			VulkanContextH2M::GetInstance();

		initInfo.PhysicalDevice =
			currentDevice
			->GetPhysicalDevice()
			->GetVulkanPhysicalDevice();

		initInfo.Device =
			device;

		initInfo.QueueFamily =
			currentDevice
			->GetPhysicalDevice()
			->GetQueueFamilyIndices()
			.Graphics;

		initInfo.Queue =
			currentDevice->GetGraphicsQueue();

		initInfo.DescriptorPool =
			m_DescriptorPool;

		initInfo.MinImageCount = 2;

		initInfo.ImageCount =
			app->GetWindow()
			->GetSwapChain()
			.GetImageCount();

		initInfo.CheckVkResultFn =
			check_vk_result;


		VkRenderPass renderPass =
			app->GetWindow()
			->GetSwapChain()
			.GetRenderPass();


		// ImGui 1.92 creates its pipeline for this render pass; without it the pipeline is VK_NULL_HANDLE
		initInfo.PipelineInfoMain.RenderPass = renderPass;

		ImGui_ImplVulkan_Init(
			&initInfo
		);

		// Upload fonts
		{
			m_CommandBuffer =
				currentDevice->GetCommandBuffer(true);

			// ImGui_ImplVulkan_CreateFontsTexture();

			currentDevice->FlushCommandBuffer(
				m_CommandBuffer
			);

			// ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void VulkanImGuiLayerH2M::OnDetach()
	{
		auto device =
			VulkanContextH2M::GetCurrentDevice()
			->GetVulkanDevice();


		if (device != VK_NULL_HANDLE)
		{
			check_vk_result(
				vkDeviceWaitIdle(device)
			);
		}

		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();

		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(
				device,
				m_DescriptorPool,
				nullptr
			);

			m_DescriptorPool = VK_NULL_HANDLE;
		}
	}

	void VulkanImGuiLayerH2M::Begin()
	{
		ImGuiIO& io = ImGui::GetIO();


		float time =
			(float)glfwGetTime();


		io.DeltaTime =
			m_Time > 0.0f
			?
			(time - m_Time)
			:
			(1.0f / 60.0f);


		m_Time = time;

		ImGui_ImplVulkan_NewFrame();

		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();


		// Required once per frame: without it ImGuizmo draws the gizmo but never registers hovering/dragging
		ImGuizmo::BeginFrame();
	}

	void VulkanImGuiLayerH2M::End()
	{
		ImGuiIO& io = ImGui::GetIO();


		Application* app =
			Application::Get();


		io.DisplaySize =
			ImVec2(
				(float)app->GetWindow()->GetWidth(),
				(float)app->GetWindow()->GetHeight()
			);

		ImGui::Render();

		/*
			The actual Vulkan draw call should happen
			inside your renderer command buffer:

			ImGui_ImplVulkan_RenderDrawData(
				ImGui::GetDrawData(),
				commandBuffer
			);

			This layer should not submit directly because
			H2M controls the render pass lifecycle.
		*/


		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();

			ImGui::RenderPlatformWindowsDefault();
		}
	}

	void VulkanImGuiLayerH2M::OnUpdate(TimestepH2M ts)
	{
	}

	void VulkanImGuiLayerH2M::OnEvent(EventH2M& event)
	{
	}

	void VulkanImGuiLayerH2M::OnImGuiRender()
	{
	}

}
