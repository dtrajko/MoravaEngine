/**
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

	struct VulkanAllocatorData
	{
		VmaAllocator Allocator;
		uint64_t TotalAllocatedBytes = 0;
	};

	static VulkanAllocatorData* s_Data = nullptr;


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

	VmaAllocation VulkanAllocatorH2M::AllocateBuffer(VkBufferCreateInfo bufferCreateInfo, VmaMemoryUsage usage, VkBuffer& outBuffer)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateBuffer(s_Data->Allocator, &bufferCreateInfo, &allocCreateInfo, &outBuffer, &allocation, nullptr);

		// TODO: Tracking
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		HZ_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating buffer; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			HZ_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}

		return allocation;
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
		VK_CHECK_RESULT_H2M(vkAllocateMemory(m_Device->GetVulkanDevice(), &memAlloc, nullptr, dest));
	}

	VmaAllocation VulkanAllocatorH2M::AllocateImage(VkImageCreateInfo imageCreateInfo, VmaMemoryUsage usage, VkImage& outImage)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = usage;

		VmaAllocation allocation;
		vmaCreateImage(s_Data->Allocator, &imageCreateInfo, &allocCreateInfo, &outImage, &allocation, nullptr);

		// TODO: Tracking
		VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(s_Data->Allocator, allocation, &allocInfo);
		HZ_ALLOCATOR_LOG("VulkanAllocator ({0}): allocating image; size = {1}", m_Tag, Utils::BytesToString(allocInfo.size));

		{
			s_Data->TotalAllocatedBytes += allocInfo.size;
			HZ_ALLOCATOR_LOG("VulkanAllocator ({0}): total allocated since start is {1}", m_Tag, Utils::BytesToString(s_Data->TotalAllocatedBytes));
		}
		return allocation;
	}

	void VulkanAllocatorH2M::Free(VmaAllocation allocation)
	{
		vmaFreeMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocatorH2M::DestroyImage(VkImage image, VmaAllocation allocation)
	{
		H2M_CORE_ASSERT(image);
		H2M_CORE_ASSERT(allocation);
		vmaDestroyImage(s_Data->Allocator, image, allocation);
	}

	void VulkanAllocatorH2M::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
	{
		H2M_CORE_ASSERT(buffer);
		H2M_CORE_ASSERT(allocation);
		vmaDestroyBuffer(s_Data->Allocator, buffer, allocation);
	}

	void VulkanAllocatorH2M::UnmapMemory(VmaAllocation allocation)
	{
		vmaUnmapMemory(s_Data->Allocator, allocation);
	}

	void VulkanAllocatorH2M::DumpStats()
	{
	}

	GPUMemoryStatsH2M VulkanAllocatorH2M::GetStats()
	{
		return GPUMemoryStatsH2M();
	}

	void VulkanAllocatorH2M::Init(RefH2M<VulkanDeviceH2M> device)
	{
		s_Data = new VulkanAllocatorData();

		// Initialize VulkanMemoryAllocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = device->GetPhysicalDevice()->GetVulkanPhysicalDevice();
		allocatorInfo.device = device->GetVulkanDevice();
		allocatorInfo.instance = VulkanContextH2M::GetInstance();

		vmaCreateAllocator(&allocatorInfo, &s_Data->Allocator);
	}

	void VulkanAllocatorH2M::Shutdown()
	{
		vmaDestroyAllocator(s_Data->Allocator);

		delete s_Data;
		s_Data = nullptr;
	}

	VmaAllocator& VulkanAllocatorH2M::GetVMAAllocator()
	{
		return s_Data->Allocator;
	}

}
