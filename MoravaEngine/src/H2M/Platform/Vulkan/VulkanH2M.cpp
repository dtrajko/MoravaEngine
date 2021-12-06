/**
 *
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "VulkanH2M.h"

#include "VulkanContextH2M.h"
#include "VulkanDiagnosticsH2M.h"
#include "H2M/Renderer/RendererH2M.h"


namespace H2M::Utils
{

	static const char* StageToString(VkPipelineStageFlagBits stage)
	{
		switch (stage)
		{
		case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT";
		case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: return "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT";
		}
		H2M_CORE_ASSERT(false);
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
			// vkGetQueueCheckpointDataNV(::Hazel::VulkanContextH2M::GetCurrentDevice()->GetGraphicsQueue(), &retrievedCount, data); // unresolved external symbol vkGetQueueCheckpointDataNV
			H2M_CORE_ERROR("RetrieveDiagnosticCheckpoints (Graphics Queue):");
			for (uint32_t i = 0; i < retrievedCount; i++)
			{
				VulkanCheckpointDataH2M* checkpoint = (VulkanCheckpointDataH2M*)data[i].pCheckpointMarker;
				H2M_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		{
			const uint32_t checkpointCount = 4;
			VkCheckpointDataNV data[checkpointCount];
			for (uint32_t i = 0; i < checkpointCount; i++)
				data[i].sType = VK_STRUCTURE_TYPE_CHECKPOINT_DATA_NV;

			uint32_t retrievedCount = checkpointCount;
			// vkGetQueueCheckpointDataNV(::Hazel::VulkanContextH2M::GetCurrentDevice()->GetComputeQueue(), &retrievedCount, data); // unresolved external symbol vkGetQueueCheckpointDataNV
			H2M_CORE_ERROR("RetrieveDiagnosticCheckpoints (Compute Queue):");
			for (uint32_t i = 0; i < retrievedCount; i++)
			{
				VulkanCheckpointDataH2M* checkpoint = (VulkanCheckpointDataH2M*)data[i].pCheckpointMarker;
				H2M_CORE_ERROR("Checkpoint: {0} (stage: {1})", checkpoint->Data, StageToString(data[i].stage));
			}
		}
		__debugbreak();
	}

	// A duplicate of DumpGPUInfo() in HazelRenderer
	void DumpGPUInfoDuplicate()
	{
		auto& caps = RendererH2M::GetCapabilities();
		Log::GetLogger()->trace("GPU Info:");
		Log::GetLogger()->trace("  Vendor: {0}", caps.Vendor);
		Log::GetLogger()->trace("  Device: {0}", caps.Device);
		Log::GetLogger()->trace("  Version: {0}", caps.Version);
	}

}
