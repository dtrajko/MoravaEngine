#pragma once

#include "Hazel/Platform/Vulkan/Vulkan.h"
#include "Hazel/Platform/Vulkan/VulkanDevice.h"

#include <string>


namespace Hazel {

	struct GPUMemoryStatsHazelLegacy
	{
		uint64_t Used = 0;
		uint64_t Free = 0;
	};

	class VulkanAllocatorHazelLegacy
	{
	public:
		VulkanAllocatorHazelLegacy() = default;
		VulkanAllocatorHazelLegacy(const std::string& tag);
		VulkanAllocatorHazelLegacy(const Ref<VulkanDevice>& device, const std::string& tag = "");
		~VulkanAllocatorHazelLegacy();

		void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	private:
		Ref<VulkanDevice> m_Device;
		std::string m_Tag;
	};

}
