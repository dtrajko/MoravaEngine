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
			// vkGetQueueCheckpointDataNV(::Hazel::VulkanContext::GetCurrentDevice()->GetGraphicsQueue(), &retrievedCount, data); // unresolved external symbol vkGetQueueCheckpointDataNV
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

	// A duplicate of DumpGPUInfo() in HazelRenderer
	void DumpGPUInfoDuplicate()
	{
		auto& caps = HazelRenderer::GetCapabilities();
		Log::GetLogger()->trace("GPU Info:");
		Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
		Log::GetLogger()->trace("  Device: {0}", caps.Device);
		Log::GetLogger()->trace("  Version: {0}", caps.Version);
	}

}
