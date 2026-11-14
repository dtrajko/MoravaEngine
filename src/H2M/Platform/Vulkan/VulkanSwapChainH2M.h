/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/AssertH2M.h"

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"
#include "H2M/Platform/Vulkan/VulkanAllocatorH2M.h"

#include <vector>

struct GLFWwindow;


namespace H2M
{

	class VulkanSwapChainH2M
	{
	public:
		VulkanSwapChainH2M() = default;

		void Init(VkInstance instance, const RefH2M<VulkanDeviceH2M>& device);
		void InitSurface(GLFWwindow* windowHandle);
		void Create(uint32_t* width, uint32_t* height, bool vsync = false);
		
		void OnResize(uint32_t width, uint32_t height);
		// VSync on or off while the window is open: the swapchain is made again (the GPU waits first)
		void SetVSync(bool vsync);

		void BeginFrame();
		void Present();

		uint32_t GetImageCount() const { return m_ImageCount; }
		// VSync as asked for (Create), and the present mode the swapchain got: FIFO waits for the display's refresh
		// (also when VSync is off but the surface has no other mode), MAILBOX and IMMEDIATE don't
		bool IsVSyncRequested() const { return m_VSync; }
		VkPresentModeKHR GetPresentMode() const { return m_PresentMode; }

		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }

		VkRenderPass GetRenderPass() { return m_RenderPass; }

		VkFramebuffer GetCurrentFramebuffer() { return GetFramebuffer(m_CurrentBufferIndex); }
		// The frame's command buffer: its frame slot's (frames in flight: the slot alternates, the image is the driver's choice)
		VkCommandBuffer GetCurrentDrawCommandBuffer() { return GetDrawCommandBuffer(m_FrameSlot); }
		// The frame slot (0 or 1 with two frames in flight, always 0 with one): what is kept per frame on the GPU
		// (the profiler's queries) is indexed by it. The slot's last frame has finished when the slot comes around again
		uint32_t GetCurrentFrameSlot() const { return m_FrameSlot; }
		uint32_t GetFramesInFlight() const { return m_FramesInFlight; }

		VkFormat GetColorFormat() { return m_ColorFormat; }

		uint32_t GetCurrentBufferIndex() const { return m_CurrentBufferIndex; }
		VkFramebuffer GetFramebuffer(uint32_t index)
		{
			H2M_CORE_ASSERT(index < m_ImageCount);
			return m_Framebuffers[index];
		}
		VkCommandBuffer GetDrawCommandBuffer(uint32_t index)
		{
			H2M_CORE_ASSERT(index < m_ImageCount);
			return m_DrawCommandBuffers[index];
		}

		void Cleanup();
	private:
		VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
		VkResult QueuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);

		void CreateFramebuffer();
		void CreateDepthStencil();
		void CreateDrawBuffers();
		void FindImageFormatAndColorSpace();

	private:
		VkInstance m_Instance;
		RefH2M<VulkanDeviceH2M> m_Device;
		VulkanAllocatorH2M m_Allocator;

		bool m_VSync = false;
		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;

		VkFormat m_ColorFormat;
		VkColorSpaceKHR m_ColorSpace;

		VkSwapchainKHR m_SwapChain = nullptr;
		uint32_t m_ImageCount = 0;
		std::vector<VkImage> m_Images;

		struct SwapChainBuffer
		{
			VkImage image;
			VkImageView view;
		};
		std::vector<SwapChainBuffer> m_Buffers;

		VkFormat m_DepthBufferFormat;
		struct
		{
			VkImage Image;
			VkDeviceMemory DeviceMemory;
			VmaAllocation MemoryAlloc;
			VkImageView ImageView;
		} m_DepthStencil;

		std::vector<VkFramebuffer> m_Framebuffers;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_DrawCommandBuffers;

		// Swap chain: signaled when the acquired image can be rendered to, one per frame slot (a slot's last frame has
		// waited for it by the time the slot comes around again)
		std::vector<VkSemaphore> m_AcquireSemaphores;

		// Command buffer: signaled when rendering is done, waited on by presentation. One per swapchain image,
		// indexed by the acquired image: a presentation may keep using its semaphore until that image is acquired again.
		std::vector<VkSemaphore> m_RenderCompleteSemaphores;

		// One per frame slot: signaled when the slot's frame is done on the GPU
		std::vector<VkFence> m_WaitFences;

		uint32_t m_FramesInFlight = 1;
		uint32_t m_FrameSlot = 0;

		VkRenderPass m_RenderPass;
		uint32_t m_CurrentBufferIndex = 0;

		uint32_t m_QueueNodeIndex = UINT32_MAX;
		uint32_t m_Width = 0, m_Height = 0;

		VkSurfaceKHR m_Surface;

		friend class VulkanContext;
	};
}
