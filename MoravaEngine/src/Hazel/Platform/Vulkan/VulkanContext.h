#pragma once

#include "Hazel/Renderer/HazelRenderer.h"

#include "Vulkan.h"
#include "VulkanDevice.h"
#include "VulkanAllocator.h"
#include "VulkanSwapChain.h"

#include "Core/Window.h"


struct GLFWwindow;

namespace Hazel {

	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext(Window* window);
		virtual ~VulkanContext();

		virtual void Create() override;
		virtual void SwapBuffers() override;

		virtual void OnResize(uint32_t width, uint32_t height) override;

		virtual void BeginFrame() override;

		Ref<VulkanDevice> GetDevice() { return m_Device; }
		VulkanSwapChain& GetSwapChain() { return m_SwapChain; }

		static VkInstance GetInstance() { return s_VulkanInstance; }

		static Ref<VulkanContext> Get() { return Ref<VulkanContext>(HazelRenderer::GetContext()); }
		static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		Window* m_Window;

		// Devices
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice> m_Device;

		// Vulkan instance
		inline static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache;

		VulkanAllocator m_Allocator;
		VulkanSwapChain m_SwapChain;
	};
}
