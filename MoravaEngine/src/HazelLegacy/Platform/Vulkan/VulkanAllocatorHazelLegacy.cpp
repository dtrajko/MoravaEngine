#include "VulkanAllocatorHazelLegacy.h"

#include "Hazel/Platform/Vulkan/VulkanContext.h"
#include "Hazel/Utilities/StringUtils.h"

#include "Core/Log.h"


#if HZ_LOG_RENDERER_ALLOCATIONS
#define HZ_ALLOCATOR_LOG(...) HZ_CORE_TRACE(__VA_ARGS__)
#else
#define HZ_ALLOCATOR_LOG(...)
#endif

namespace Hazel {

	VulkanAllocatorHazelLegacy::VulkanAllocatorHazelLegacy(const Ref<VulkanDevice>& device, const std::string& tag)
		: m_Device(device), m_Tag(tag)
	{
	}

	VulkanAllocatorHazelLegacy::VulkanAllocatorHazelLegacy(const std::string& tag)
		: m_Device(VulkanContext::GetCurrentDevice()), m_Tag(tag)
	{
	}

	VulkanAllocatorHazelLegacy::~VulkanAllocatorHazelLegacy()
	{
	}

	void VulkanAllocatorHazelLegacy::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		HZ_CORE_ASSERT(m_Device);

		// TODO: Tracking
		Log::GetLogger()->trace("VulkanAllocator ({0}): allocating {1} bytes", m_Tag, requirements.size);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}

}