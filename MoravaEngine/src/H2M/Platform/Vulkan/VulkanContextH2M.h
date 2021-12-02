#pragma once

#include "H2M/Renderer/RendererH2M.h"

#include "VulkanH2M.h"
#include "VulkanDeviceH2M.h"
#include "VulkanAllocatorH2M.h"
#include "VulkanSwapChainH2M.h"

#include "Core/Window.h"


struct GLFWwindow;

namespace H2M
{

	class VulkanContext : public RendererContext
	{
	public:
		VulkanContext(Window* window);
		virtual ~VulkanContext();

		virtual void Create() override;
		virtual void SwapBuffers() override;

		virtual void OnResize(uint32_t width, uint32_t height) override;

		virtual void BeginFrame() override;

		RefH2M<VulkanDevice> GetDevice() { return m_Device; }
		VulkanSwapChain& GetSwapChain() { return m_SwapChain; }

		static VkInstance GetInstance() { return s_VulkanInstance; }

		static RefH2M<VulkanContext> Get();
		static RefH2M<VulkanDevice> GetCurrentDevice() { return Get()->GetDevice(); }

	private:
		Window* m_Window;

		// Devices
		RefH2M<VulkanPhysicalDevice> m_PhysicalDevice;
		RefH2M<VulkanDevice> m_Device;

		// Vulkan instance
		inline static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache;

		VulkanAllocator m_Allocator;
		VulkanSwapChain m_SwapChain;

	};
}
