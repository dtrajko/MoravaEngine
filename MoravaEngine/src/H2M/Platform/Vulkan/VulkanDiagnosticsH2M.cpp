/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

// #define _CRT_SECURE_NO_WARNINGS

#include "VulkanDiagnosticsH2M.h"


namespace H2M::Utils
{

	static std::vector<VulkanCheckpointDataH2M> s_CheckpointStorage(1024);
	static uint32_t s_CheckpointStorageIndex = 0;

	void SetVulkanCheckpoint(VkCommandBuffer commandBuffer, const std::string& data)
	{
		s_CheckpointStorageIndex = (s_CheckpointStorageIndex + 1) % 1024;
		VulkanCheckpointDataH2M& checkpoint = s_CheckpointStorage[s_CheckpointStorageIndex];
		memset(checkpoint.Data, 0, sizeof(checkpoint.Data));
		strcpy_s(checkpoint.Data, data.data());
		// vkCmdSetCheckpointNV(commandBuffer, &checkpoint);
	}

}
