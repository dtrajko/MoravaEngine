/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#pragma once

#include "H2M/Platform/Vulkan/VulkanH2M.h"
#include "H2M/Platform/Vulkan/VulkanDeviceH2M.h"

#include "VulkanMemoryAllocator/vk_mem_alloc.h"

#include <string>


namespace H2M
{

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
		VulkanAllocatorH2M(const RefH2M<VulkanDeviceH2M>& device, const std::string& tag = "");
		~VulkanAllocatorH2M();

		VmaAllocation AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer);
		VmaAllocation AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage);
		void Free(VmaAllocation allocation);
		void DestroyImage(VkImage image, VmaAllocation allocation);
		void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

		template<typename T>
		T* MapMemory(VmaAllocation allocation)
		{
			T* mappedMemory;
			vmaMapMemory(VulkanAllocator::GetVMAAllocator(), allocation, (void**)&mappedMemory);
			return mappedMemory;
		}

		void UnmapMemory(VmaAllocation allocation);

		static void DumpStats();
		static GPUMemoryStatsH2M GetStats();

		void Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		static void Init(RefH2M<VulkanDeviceH2M> device);
		static void Shutdown();

		static VmaAllocator& GetVMAAllocator();

	private:
		RefH2M<VulkanDeviceH2M> m_Device;
		std::string m_Tag;

	};

}
