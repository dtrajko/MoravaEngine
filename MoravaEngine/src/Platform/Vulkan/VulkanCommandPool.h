#pragma once

#include <vulkan/vulkan.h>


class VulkanCommandPool
{
public:
	VulkanCommandPool(VkDevice device);
	~VulkanCommandPool();

private:
	VkDevice m_Device;
	VkCommandPool m_CommandPool;

};
