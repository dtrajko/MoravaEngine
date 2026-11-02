/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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
