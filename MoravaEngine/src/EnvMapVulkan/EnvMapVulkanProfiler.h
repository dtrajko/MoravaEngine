#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


/**
 * Frame profiler of SceneEnvMapVulkan: what the GPU does in a frame, pass by pass, and how long it takes.
 *
 * The renderer marks the parts of its frame with scopes (BeginScope / EndScope, nested): "Sun Shadows" with a scope per
 * cascade, "Scene" with "Opaque Meshes", "Water Surface" and so on. For each scope the GPU writes a timestamp where it
 * starts and where it ends (a timestamp query), and the CPU counts what was recorded in it: draw calls, triangles and
 * render passes (CountDraw, CountRenderPass). A top-level scope also gets the GPU's pipeline statistics (a pipeline
 * statistics query): vertices, vertex and fragment shader invocations, primitives that reached the rasterizer.
 *
 * The GPU runs the frame after it was recorded, so its results are read when the same swapchain image's command buffer
 * comes around again (its fence was waited for by then): the panel shows the last finished frame. Each swapchain image
 * has its own query pools and its own record of the scopes.
 *
 * Rules (Vulkan's): a top-level scope must begin and end outside a render pass (a pipeline statistics query can't span
 * one partially), and a scope must end in the command buffer it began in.
 */
class EnvMapVulkanProfiler
{
public:
	// Pipeline statistics of a top-level scope, in the order Vulkan writes them (see StatisticFlags)
	enum Statistic
	{
		InputVertices = 0,       // vertices read by the input assembler
		InputPrimitives,         // primitives (triangles, lines) assembled
		VertexShaderInvocations,
		ClippingInvocations,     // primitives that reached the clipping stage
		ClippingPrimitives,      // primitives that left it (after culling and clipping): what was rasterized
		FragmentShaderInvocations,
		StatisticCount
	};

	struct ScopeResult
	{
		std::string Name;
		const char* Description = nullptr; // what the pass does (the panel's tooltip)
		int Depth = 0;                     // 0: top level
		double GpuMs = 0.0;                // this frame's
		double SmoothedGpuMs = 0.0;        // averaged over the last frames (steadier to read)
		uint32_t Draws = 0;                // draw calls recorded in it (with its children)
		uint64_t Triangles = 0;
		uint32_t RenderPasses = 0;
		uint32_t TargetWidth = 0;          // the size of its last render pass's target (0: none)
		uint32_t TargetHeight = 0;
		bool HasStatistics = false;
		std::array<uint64_t, StatisticCount> Statistics{};
	};

	struct FrameResults
	{
		uint64_t FrameNumber = 0;
		bool Valid = false;          // timestamps are supported and this frame's were read
		double GpuMs = 0.0;          // from the first timestamp to the last
		double SmoothedGpuMs = 0.0;
		uint32_t Draws = 0;          // in the whole frame
		uint64_t Triangles = 0;
		uint32_t RenderPasses = 0;
		std::vector<ScopeResult> Scopes; // in recording order (a parent before its children)
	};

	static constexpr uint32_t HistorySize = 240;

	static void Init();
	static void Shutdown();

	// The start and the end of a frame's recording (the first and last command of the frame's command buffer; outside
	// render passes). BeginFrame reads the results of the frame that last used this swapchain image, then resets.
	static void BeginFrame(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex);
	static void EndFrame(VkCommandBuffer commandBuffer);

	// statistics: false for a top-level scope that runs secondary command buffers (vkCmdExecuteCommands can't run while
	// a query is active, unless the device has the inheritedQueries feature)
	static void BeginScope(VkCommandBuffer commandBuffer, const std::string& name, const char* description = nullptr, bool statistics = true);
	static void EndScope(VkCommandBuffer commandBuffer);

	// Counted in the innermost open scope (and its parents)
	static void CountDraw(uint64_t triangles);
	static void CountRenderPass(uint32_t width, uint32_t height);

	static const FrameResults& GetResults() { return s_Results; }

	// CPU: the time between frames (what the frame rate is made of: recording, the GPU's work, waiting, the UI)
	static double GetCpuFrameMs() { return s_CpuFrameMs; }
	static double GetSmoothedCpuFrameMs() { return s_SmoothedCpuFrameMs; }
	static double GetFps() { return s_SmoothedCpuFrameMs > 0.0 ? 1000.0 / s_SmoothedCpuFrameMs : 0.0; }
	// The last HistorySize frames, oldest first (ms)
	static const std::array<float, HistorySize>& GetCpuHistory() { return s_CpuHistory; }
	static const std::array<float, HistorySize>& GetGpuHistory() { return s_GpuHistory; }
	static uint32_t GetHistoryOffset() { return s_HistoryOffset; } // where the oldest value is

	static bool IsSupported() { return s_TimestampsSupported; }
	static bool AreStatisticsSupported() { return s_StatisticsSupported; }

	// A scope that ends at the end of the C++ block it is declared in
	class Scope
	{
	public:
		Scope(VkCommandBuffer commandBuffer, const std::string& name, const char* description = nullptr, bool statistics = true)
			: m_CommandBuffer(commandBuffer)
		{
			BeginScope(commandBuffer, name, description, statistics);
		}
		~Scope() { EndScope(m_CommandBuffer); }
		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;

	private:
		VkCommandBuffer m_CommandBuffer;
	};

private:
	// A scope as recorded (its queries are read when the frame is finished)
	struct RecordedScope
	{
		std::string Name;
		const char* Description = nullptr;
		int Depth = 0;
		int Parent = -1;
		uint32_t BeginQuery = 0;    // timestamp queries
		uint32_t EndQuery = 0;
		int StatisticsQuery = -1;   // pipeline statistics query (top level only, while there are free ones)
		uint32_t Draws = 0;
		uint64_t Triangles = 0;
		uint32_t RenderPasses = 0;
		uint32_t TargetWidth = 0;
		uint32_t TargetHeight = 0;
	};

	struct FrameSlot
	{
		VkQueryPool Timestamps = VK_NULL_HANDLE;
		VkQueryPool Statistics = VK_NULL_HANDLE;
		std::vector<RecordedScope> Scopes;
		uint32_t TimestampsUsed = 0;
		uint32_t StatisticsUsed = 0;
		uint64_t FrameNumber = 0;
		bool Recorded = false; // holds a recorded frame whose results weren't read yet
	};

	static void CreateSlot(FrameSlot& slot);
	static void ReadSlot(FrameSlot& slot);
	static double Smooth(const std::string& key, double value);

	static constexpr uint32_t MaxTimestamps = 512;     // per frame: 2 per scope, 2 for the frame
	static constexpr uint32_t MaxStatisticsQueries = 32; // per frame: one per top-level scope

	static inline std::vector<FrameSlot> s_Slots;
	static inline FrameSlot* s_Recording = nullptr;   // the slot of the frame being recorded
	static inline std::vector<int> s_OpenScopes;      // indices into s_Recording->Scopes, innermost last
	static inline uint64_t s_FrameNumber = 0;
	static inline bool s_TimestampsSupported = false;
	static inline bool s_StatisticsSupported = false;
	static inline uint64_t s_TimestampMask = ~0ull;   // the queue's valid timestamp bits
	static inline double s_TimestampPeriodNs = 1.0;   // nanoseconds per timestamp tick

	static inline FrameResults s_Results;
	static inline std::unordered_map<std::string, double> s_Smoothed;

	static inline std::chrono::steady_clock::time_point s_LastFrameStart;
	static inline bool s_HasLastFrameStart = false;
	static inline double s_CpuFrameMs = 0.0;
	static inline double s_SmoothedCpuFrameMs = 0.0;
	static inline std::array<float, HistorySize> s_CpuHistory{};
	static inline std::array<float, HistorySize> s_GpuHistory{};
	static inline uint32_t s_HistoryOffset = 0;
};
