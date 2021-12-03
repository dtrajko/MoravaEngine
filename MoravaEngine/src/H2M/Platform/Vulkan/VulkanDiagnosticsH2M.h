#pragma once

#include "VulkanH2M.h"


namespace H2M::Utils
{

	struct VulkanCheckpointDataH2M
	{
		char Data[64];
	};

	void SetVulkanCheckpoint(VkCommandBuffer commandBuffer, const std::string& data);

}
