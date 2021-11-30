#pragma once

#include "H2M/Platform/Vulkan/Vulkan.h"
#include "H2M/Platform/Vulkan/VulkanDevice.h"

#include <string>


namespace Hazel {

	struct GPUMemoryStatsH2M
	{
		uint64_t Used = 0;
		uint64_t Free = 0;
	};

	class VulkanAllocatorH2M
	{
	public:
		VulkanAllocatorH2M() = default;
		VulkanAllocatorH2M(const std::string& tag);
		VulkanAllocatorH2M(const Ref<VulkanDevice>& device, const std::string& tag = "");
		~VulkanAllocatorH2M();

		void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	private:
		Ref<VulkanDevice> m_Device;
		std::string m_Tag;
	};

}
