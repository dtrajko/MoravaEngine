#pragma once

#include <vulkan/vulkan.h>


namespace H2M
{
	/**
	 * Freeing a vertex or index buffer: a frame in flight may still draw with it, so it goes a few frames later. Destructors
	 * only queue here, so they are safe wherever the last reference happens to go.
	 */
	namespace VulkanBufferReleaseH2M
	{
		void Queue(VkBuffer buffer, VkDeviceMemory memory);
		void OnFrame();
		// All of it now (the device goes soon); a buffer that goes after this is left to the device's end
		void Shutdown();
	}
}
