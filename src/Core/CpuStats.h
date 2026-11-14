#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>


/**
 * The CPU's time in the stages of a frame: the main loop's (Application: events, the update, the renderer's draw,
 * present...) and the stages inside them (the renderer: the models submitted, the passes recorded; the game: scripts,
 * physics, its UI). A stage adds its time each frame it runs; EndFrame (Application, once a frame) smooths each stage's
 * time for the displays (the editor's Stats panel, the player's F3 overlay) and, while --frame-stats collects, adds it
 * to the run's totals that LogTotals writes at exit.
 *
 * Main thread only.
 */
namespace CpuStats
{
	// The main loop's stages (Application::Run), in their order. The renderer's and the game's stages name theirs as parent
	inline constexpr const char* StageEvents = "Events";
	inline constexpr const char* ParentUpdate = "Update (the editor, the game)"; // the layers' update: Play's game loop in it
	inline constexpr const char* StageSceneUpdate = "Scene update";
	inline constexpr const char* StageSubmit = "Render queue (ImGui submitted)";
	inline constexpr const char* ParentDraw = "Draw (the renderer)";            // the frame recorded, the UI with it
	inline constexpr const char* StagePresent = "Present (waits for the GPU, VSync)";
	inline constexpr const char* StageFrameLimit = "Frame rate limit (waits)";

	struct Stage
	{
		std::string Name;
		std::string Parent;      // the stage it is part of (empty: a stage of the main loop)
		double FrameMs = 0.0;    // the last frame's (0: it didn't run)
		double SmoothedMs = 0.0; // averaged over the last frames (steadier to read)
		double TotalMs = 0.0;    // --frame-stats: over the collected frames
	};

	// This frame's time of a stage (added up: a stage that runs twice in a frame, like the physics step, counts both)
	void Add(const char* name, double ms, const char* parent = nullptr);
	// The end of a frame: the smoothed values; collecting: --frame-stats counts this frame (past its warmup)
	void EndFrame(bool collecting);

	// In the order they first ran, the main loop's first
	const std::vector<Stage>& GetStages();
	uint64_t GetCollectedFrames();
	// --frame-stats, at exit: each stage's average over the collected frames
	void LogTotals();

	// Times the stages of a block one after the other: Next ends one (and starts the next)
	class Timer
	{
	public:
		explicit Timer(const char* parent = nullptr) : m_Parent(parent), m_Start(std::chrono::steady_clock::now()) {}
		void Next(const char* finishedStage)
		{
			const auto now = std::chrono::steady_clock::now();
			Add(finishedStage, std::chrono::duration<double, std::milli>(now - m_Start).count(), m_Parent);
			m_Start = now;
		}

	private:
		const char* m_Parent;
		std::chrono::steady_clock::time_point m_Start;
	};
}
