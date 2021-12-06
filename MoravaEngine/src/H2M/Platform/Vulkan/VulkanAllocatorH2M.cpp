/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanAllocatorH2M.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"
#include "H2M/Utilities/StringUtilsH2M.h"

#include "Core/Log.h"


#if HZ_LOG_RENDERER_ALLOCATIONS
#define HZ_ALLOCATOR_LOG(...) HZ_CORE_TRACE(__VA_ARGS__)
#else
#define HZ_ALLOCATOR_LOG(...)
#endif

namespace H2M
{

	VulkanAllocatorH2M::VulkanAllocatorH2M(const RefH2M<VulkanDeviceH2M>& device, const std::string& tag)
		: m_Device(device), m_Tag(tag)
	{
	}

	VulkanAllocatorH2M::VulkanAllocatorH2M(const std::string& tag)
		: m_Device(VulkanContextH2M::GetCurrentDevice()), m_Tag(tag)
	{
	}

	VulkanAllocatorH2M::~VulkanAllocatorH2M()
	{
	}

	void VulkanAllocatorH2M::Allocate(VkMemoryRequirements requirements, VkDeviceMemory* dest, VkMemoryPropertyFlags flags /*= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT*/)
	{
		H2M_CORE_ASSERT(m_Device);

		// TODO: Tracking
		Log::GetLogger()->trace("VulkanAllocator ({0}): allocating {1} bytes", m_Tag, requirements.size);

		VkMemoryAllocateInfo memAlloc = {};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = requirements.size;
		memAlloc.memoryTypeIndex = m_Device->GetPhysicalDevice()->GetMemoryTypeIndex(requirements.memoryTypeBits, flags);
		VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}

}
