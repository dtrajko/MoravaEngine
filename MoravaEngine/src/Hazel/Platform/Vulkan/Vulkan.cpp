#include "Vulkan.h"

#include "VulkanContext.h"
#include "VulkanDiagnostics.h"
#include "Hazel/Renderer/HazelRenderer.h"


namespace Hazel::Utils {

	static const char* StageToString(VkPipelineStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT";
		case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT";
		}
		HZ_CORE_ASSERT(false);
		return nullptr;
	}

	void RetrieveDiagnosticCheckpoints()
	{
		{
			const uint32_t checkpointCount = 4;
			VkCheckpointDataNV data[checkpointCount];
			for (uint32_t i = 0; i < checkpointCount; i++)
				data[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;

			uint32_t retrievedCount = checkpointCount;
			// vkGetQueueCheckpointDataNV(::Hazel::VulkanContext::GetCurrentDevice()->GetQueue(), &retrievedCount, data); // unresolved external symbol vkGetQueueCheckpointDataNV
			HZ_CORE_ERROR("RetrieveDiagnosticCheckpoints (Graphics Queue):");
			for (uint32_t i = 0; i < retrievedCount; i++)
			{
				VulkanCheckpointData* checkpoint = (VulkanCheckpointData*)data[i].pCheckpointMarker;
				HZ_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		{
			const uint32_t checkpointCount = 4;
			VkCheckpointDataNV data[checkpointCount];
			for (uint32_t i = 0; i < checkpointCount; i++)
				data[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;

			uint32_t retrievedCount = checkpointCount;
			// vkGetQueueCheckpointDataNV(::Hazel::VulkanContext::GetCurrentDevice()->GetComputeQueue(), &retrievedCount, data); // unresolved external symbol vkGetQueueCheckpointDataNV
			HZ_CORE_ERROR("RetrieveDiagnosticCheckpoints (Compute Queue):");
			for (uint32_t i = 0; i < retrievedCount; i++)
			{
				VulkanCheckpointData* checkpoint = (VulkanCheckpointData*)data[i].pCheckpointMarker;
				HZ_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		__debugbreak();
	}

	void VulkanCheckResult(VkResult result)
	{
		if (result != VK_SUCCESS)
		{
			HZ_CORE_ERROR("VkResult is '{0}' in {1}:{2}", ::Hazel::Utils::VKResultToString(result), __FILE__, __LINE__);
			if (result == VK_ERROR_DEVICE_LOST)
			{
				::Hazel::Utils::RetrieveDiagnosticCheckpoints();
				::Hazel::Utils::DumpGPUInfo();
			}
			HZ_CORE_ASSERT(result == VK_SUCCESS);
		}
	}

}
