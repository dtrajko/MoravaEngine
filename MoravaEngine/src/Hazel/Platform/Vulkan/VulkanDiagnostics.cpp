#define _CRT_SECURE_NO_WARNINGS

#include "VulkanDiagnostics.h"


namespace Hazel::Utils {

	static std::vector<VulkanCheckpointData> s_CheckpointStorage(1024);
	static uint32_t s_CheckpointStorageIndex = 0;

	void SetVulkanCheckpoint(VkCommandBuffer commandBuffer, const std::string& data)
	{
		s_CheckpointStorageIndex = (s_CheckpointStorageIndex + 1) % 1024;
		VulkanCheckpointData& checkpoint = s_CheckpointStorage[s_CheckpointStorageIndex];
		memset(checkpoint.Data, 0, sizeof(checkpoint.Data));
		strcpy(checkpoint.Data, data.data());
		vkCmdSetCheckpointNV(commandBuffer, &checkpoint);
	}

}
