#include "EnvMapVulkanProfiler.h"

#include "H2M/Platform/Vulkan/VulkanContextH2M.h"

#include "Core/Log.h"

#include <algorithm>


// The statistics read for a top-level scope, in the order of EnvMapVulkanProfiler::Statistic (Vulkan writes the values
// in the order of the flag bits)
static constexpr VkQueryPipelineStatisticFlags StatisticFlags =
	VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
	VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
	VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
	VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
	VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
	VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;

static constexpr uint32_t NoQuery = ~0u;

static VkDevice GetDevice()
{
	return H2M::VulkanContextH2M::GetCurrentDevice()->GetVulkanDevice();
}

void EnvMapVulkanProfiler::Init()
{
	H2M::RefH2M<H2M::VulkanDeviceH2M> device = H2M::VulkanContextH2M::GetCurrentDevice();
	const H2M::RefH2M<H2M::VulkanPhysicalDeviceH2M>& physicalDevice = device->GetPhysicalDevice();
	const VkPhysicalDeviceLimits& limits = physicalDevice->GetLimits();

	// Timestamps: the graphics queue's family must have valid bits (timestampComputeAndGraphics says all of them do)
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->GetVulkanPhysicalDevice(), &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->GetVulkanPhysicalDevice(), &familyCount, families.data());
	const int32_t graphicsFamily = physicalDevice->GetQueueFamilyIndices().Graphics;
	const uint32_t validBits = graphicsFamily >= 0 && graphicsFamily < (int32_t)familyCount ? families[graphicsFamily].timestampValidBits : 0;

	s_TimestampsSupported = validBits > 0 && limits.timestampPeriod > 0.0f;
	s_TimestampMask = validBits >= 64 ? ~0ull : ((1ull << validBits) - 1ull);
	s_TimestampPeriodNs = limits.timestampPeriod;
	s_StatisticsSupported = device->GetEnabledFeatures().pipelineStatisticsQuery == VK_TRUE;

	Log::GetLogger()->info("Profiler: GPU timestamps {0} ({1} valid bits, {2} ns per tick), pipeline statistics {3}",
		s_TimestampsSupported ? "supported" : "NOT supported", validBits, limits.timestampPeriod, s_StatisticsSupported ? "supported" : "NOT supported");
}

void EnvMapVulkanProfiler::Shutdown()
{
	VkDevice device = GetDevice();
	for (FrameSlot& slot : s_Slots)
	{
		if (slot.Timestamps != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(device, slot.Timestamps, nullptr);
		}
		if (slot.Statistics != VK_NULL_HANDLE)
		{
			vkDestroyQueryPool(device, slot.Statistics, nullptr);
		}
	}
	s_Slots.clear();
	s_Recording = nullptr;
	s_OpenScopes.clear();
}

void EnvMapVulkanProfiler::CreateSlot(FrameSlot& slot)
{
	VkDevice device = GetDevice();
	if (s_TimestampsSupported)
	{
		VkQueryPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		info.queryCount = MaxTimestamps;
		if (vkCreateQueryPool(device, &info, nullptr, &slot.Timestamps) != VK_SUCCESS)
		{
			slot.Timestamps = VK_NULL_HANDLE;
		}
	}
	if (s_StatisticsSupported)
	{
		VkQueryPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
		info.queryCount = MaxStatisticsQueries;
		info.pipelineStatistics = StatisticFlags;
		if (vkCreateQueryPool(device, &info, nullptr, &slot.Statistics) != VK_SUCCESS)
		{
			slot.Statistics = VK_NULL_HANDLE;
		}
	}
}

double EnvMapVulkanProfiler::Smooth(const std::string& key, double value)
{
	// An exponential moving average: about the last 20 frames
	auto it = s_Smoothed.find(key);
	if (it == s_Smoothed.end())
	{
		s_Smoothed[key] = value;
		return value;
	}
	it->second += (value - it->second) * 0.05;
	return it->second;
}

// The results of the frame recorded in the slot (finished on the GPU: its command buffer's fence was waited for)
void EnvMapVulkanProfiler::ReadSlot(FrameSlot& slot)
{
	VkDevice device = GetDevice();

	std::vector<uint64_t> timestamps;
	bool timestampsRead = false;
	if (slot.Timestamps != VK_NULL_HANDLE && slot.TimestampsUsed > 0)
	{
		timestamps.resize(slot.TimestampsUsed);
		timestampsRead = vkGetQueryPoolResults(device, slot.Timestamps, 0, slot.TimestampsUsed, timestamps.size() * sizeof(uint64_t),
			timestamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS;
	}
	std::vector<uint64_t> statistics;
	bool statisticsRead = false;
	if (slot.Statistics != VK_NULL_HANDLE && slot.StatisticsUsed > 0)
	{
		statistics.resize((size_t)slot.StatisticsUsed * StatisticCount);
		statisticsRead = vkGetQueryPoolResults(device, slot.Statistics, 0, slot.StatisticsUsed, statistics.size() * sizeof(uint64_t),
			statistics.data(), StatisticCount * sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS;
	}

	auto elapsedMs = [&](uint32_t begin, uint32_t end) -> double {
		if (!timestampsRead || begin == NoQuery || end == NoQuery || begin >= timestamps.size() || end >= timestamps.size())
		{
			return 0.0;
		}
		const uint64_t start = timestamps[begin] & s_TimestampMask;
		const uint64_t stop = timestamps[end] & s_TimestampMask;
		return stop > start ? (double)(stop - start) * s_TimestampPeriodNs * 1.0e-6 : 0.0;
	};

	FrameResults results;
	results.FrameNumber = slot.FrameNumber;
	results.Valid = timestampsRead;
	results.GpuMs = elapsedMs(0, 1);
	results.SmoothedGpuMs = Smooth("#frame", results.GpuMs);
	results.Scopes.reserve(slot.Scopes.size());
	std::vector<std::string> paths(slot.Scopes.size());
	for (size_t i = 0; i < slot.Scopes.size(); i++)
	{
		const RecordedScope& recorded = slot.Scopes[i];
		paths[i] = (recorded.Parent >= 0 ? paths[recorded.Parent] + "/" : std::string()) + recorded.Name;

		ScopeResult scope;
		scope.Name = recorded.Name;
		scope.Description = recorded.Description;
		scope.Depth = recorded.Depth;
		scope.GpuMs = elapsedMs(recorded.BeginQuery, recorded.EndQuery);
		scope.SmoothedGpuMs = Smooth(paths[i], scope.GpuMs);
		scope.Draws = recorded.Draws;
		scope.Triangles = recorded.Triangles;
		scope.RenderPasses = recorded.RenderPasses;
		scope.TargetWidth = recorded.TargetWidth;
		scope.TargetHeight = recorded.TargetHeight;
		if (statisticsRead && recorded.StatisticsQuery >= 0)
		{
			scope.HasStatistics = true;
			for (int s = 0; s < StatisticCount; s++)
			{
				scope.Statistics[s] = statistics[(size_t)recorded.StatisticsQuery * StatisticCount + s];
			}
		}
		if (recorded.Depth == 0)
		{
			results.Draws += recorded.Draws;
			results.Triangles += recorded.Triangles;
			results.RenderPasses += recorded.RenderPasses;
		}
		results.Scopes.push_back(std::move(scope));
	}
	s_Results = std::move(results);

	s_GpuHistory[(s_HistoryOffset + HistorySize - 1) % HistorySize] = (float)s_Results.GpuMs;
	slot.Recorded = false;
}

void EnvMapVulkanProfiler::BeginFrame(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex)
{
	// CPU frame time: from the start of the last frame's recording to this one's
	const auto now = std::chrono::steady_clock::now();
	if (s_HasLastFrameStart)
	{
		s_CpuFrameMs = std::chrono::duration<double, std::milli>(now - s_LastFrameStart).count();
		s_SmoothedCpuFrameMs = s_SmoothedCpuFrameMs > 0.0 ? s_SmoothedCpuFrameMs + (s_CpuFrameMs - s_SmoothedCpuFrameMs) * 0.05 : s_CpuFrameMs;
		s_CpuHistory[s_HistoryOffset] = (float)s_CpuFrameMs;
		s_GpuHistory[s_HistoryOffset] = s_GpuHistory[(s_HistoryOffset + HistorySize - 1) % HistorySize]; // until this frame's is read
		s_HistoryOffset = (s_HistoryOffset + 1) % HistorySize;
	}
	s_LastFrameStart = now;
	s_HasLastFrameStart = true;

	if (swapchainImageIndex >= s_Slots.size())
	{
		s_Slots.resize(swapchainImageIndex + 1);
	}
	FrameSlot& slot = s_Slots[swapchainImageIndex];
	if (slot.Timestamps == VK_NULL_HANDLE && slot.Statistics == VK_NULL_HANDLE && (s_TimestampsSupported || s_StatisticsSupported))
	{
		CreateSlot(slot);
	}
	if (slot.Recorded)
	{
		ReadSlot(slot);
	}

	// A new frame in the slot (the reset is recorded first: outside any render pass)
	slot.Scopes.clear();
	slot.TimestampsUsed = 0;
	slot.StatisticsUsed = 0;
	slot.FrameNumber = ++s_FrameNumber;
	if (slot.Timestamps != VK_NULL_HANDLE)
	{
		vkCmdResetQueryPool(commandBuffer, slot.Timestamps, 0, MaxTimestamps);
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, slot.Timestamps, 0);
		slot.TimestampsUsed = 2; // 0: the frame's start, 1: its end
	}
	if (slot.Statistics != VK_NULL_HANDLE)
	{
		vkCmdResetQueryPool(commandBuffer, slot.Statistics, 0, MaxStatisticsQueries);
	}
	s_Recording = &slot;
	s_OpenScopes.clear();
}

void EnvMapVulkanProfiler::EndFrame(VkCommandBuffer commandBuffer)
{
	if (!s_Recording)
	{
		return;
	}
	while (!s_OpenScopes.empty())
	{
		Log::GetLogger()->warn("Profiler: scope '{0}' was not ended", s_Recording->Scopes[s_OpenScopes.back()].Name);
		EndScope(commandBuffer);
	}
	if (s_Recording->Timestamps != VK_NULL_HANDLE)
	{
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, s_Recording->Timestamps, 1);
	}
	s_Recording->Recorded = true;
	s_Recording = nullptr;
}

void EnvMapVulkanProfiler::BeginScope(VkCommandBuffer commandBuffer, const std::string& name, const char* description, bool statistics)
{
	if (!s_Recording)
	{
		return;
	}
	FrameSlot& slot = *s_Recording;
	RecordedScope scope;
	scope.Name = name;
	scope.Description = description;
	scope.Parent = s_OpenScopes.empty() ? -1 : s_OpenScopes.back();
	scope.Depth = (int)s_OpenScopes.size();
	scope.BeginQuery = scope.EndQuery = NoQuery;
	if (slot.Timestamps != VK_NULL_HANDLE && slot.TimestampsUsed + 2 <= MaxTimestamps)
	{
		scope.BeginQuery = slot.TimestampsUsed++;
		scope.EndQuery = slot.TimestampsUsed++;
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, slot.Timestamps, scope.BeginQuery);
	}
	if (statistics && scope.Depth == 0 && slot.Statistics != VK_NULL_HANDLE && slot.StatisticsUsed < MaxStatisticsQueries)
	{
		scope.StatisticsQuery = (int)slot.StatisticsUsed++;
		vkCmdBeginQuery(commandBuffer, slot.Statistics, (uint32_t)scope.StatisticsQuery, 0);
	}
	slot.Scopes.push_back(std::move(scope));
	s_OpenScopes.push_back((int)slot.Scopes.size() - 1);
}

void EnvMapVulkanProfiler::EndScope(VkCommandBuffer commandBuffer)
{
	if (!s_Recording || s_OpenScopes.empty())
	{
		return;
	}
	FrameSlot& slot = *s_Recording;
	const RecordedScope& scope = slot.Scopes[s_OpenScopes.back()];
	if (scope.StatisticsQuery >= 0)
	{
		vkCmdEndQuery(commandBuffer, slot.Statistics, (uint32_t)scope.StatisticsQuery);
	}
	if (scope.EndQuery != NoQuery)
	{
		vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, slot.Timestamps, scope.EndQuery);
	}
	s_OpenScopes.pop_back();
}

void EnvMapVulkanProfiler::CountDraw(uint64_t triangles)
{
	if (!s_Recording)
	{
		return;
	}
	for (int index : s_OpenScopes)
	{
		s_Recording->Scopes[index].Draws++;
		s_Recording->Scopes[index].Triangles += triangles;
	}
}

void EnvMapVulkanProfiler::CountRenderPass(uint32_t width, uint32_t height)
{
	if (!s_Recording)
	{
		return;
	}
	for (int index : s_OpenScopes)
	{
		RecordedScope& scope = s_Recording->Scopes[index];
		scope.RenderPasses++;
		scope.TargetWidth = width;
		scope.TargetHeight = height;
	}
}
