/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Renderer/RendererH2M.h"

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"
#include "H2M/Platform/Vulkan/VulkanSwapChainH2M.h"

#include "Core/Window.h"


struct GLFWwindow;

namespace H2M
{

	class VulkanContextH2M : public RendererContextH2M
	{
	public:
		VulkanContextH2M(Window* window);
		virtual ~VulkanContextH2M();

		virtual void Init() override;

		RefH2M<VulkanDeviceH2M> GetDevice() { return m_Device; }

		static VkInstance GetInstance() { return s_VulkanInstance; }

		static RefH2M<VulkanContextH2M> Get() { return RefH2M<VulkanContextH2M>(RendererH2M::GetContext()); }
		static RefH2M<VulkanDeviceH2M> GetCurrentDevice() { return Get()->GetDevice(); }

		// virtual void Create() override;                               // TODO: remove the method (replace with Init)
		virtual void SwapBuffers() override;                             // TODO: remove the method
		virtual void OnResize(uint32_t width, uint32_t height) override; // TODO: remove the method
		virtual void BeginFrame() override;                              // TODO: remove the method
		// VulkanSwapChainH2M& GetSwapChain() { return m_SwapChain; }    // TODO: remove the method

	private:
		Window* m_Window;

		// Devices
		RefH2M<VulkanPhysicalDeviceH2M> m_PhysicalDevice;
		RefH2M<VulkanDeviceH2M> m_Device;

		// Vulkan instance
		inline static VkInstance s_VulkanInstance;
		VkDebugReportCallbackEXT m_DebugReportCallback = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache;

		VulkanAllocatorH2M m_Allocator;
		// VulkanSwapChainH2M m_SwapChain;

	};
}
