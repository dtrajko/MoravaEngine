#include "VulkanCommandPool.h"

#include <stdexcept>


VulkanCommandPool::VulkanCommandPool(VkDevice device)
	: m_Device(device)
{
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = 0; // TODO
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool!");
	}
}

VulkanCommandPool::~VulkanCommandPool()
{
	vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
}
