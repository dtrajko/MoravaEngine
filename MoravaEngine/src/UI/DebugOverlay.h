#pragma once

#include "Audio/SceneAudio.h"

#include "Physics/ScenePhysics.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

class EnvMapVulkanScene;
class GameLoop;
class GameSystems;
struct UIDrawList;


// The running game's numbers for the debug overlay, gathered by the game's owner (the player, the editor's Play) each frame
struct DebugOverlayGame
{
	std::string SceneName;
	uint32_t Entities = 0;
	float StepsPerSecond = 0.0f; // fixed steps, over the last second of real time
	float TimeScale = 1.0f;
	double GameTime = 0.0;
	ScenePhysics::Stats Physics;
	uint32_t ScriptInstances = 0;
	uint64_t ScriptExceptions = 0;
	bool Managed = false;      // C# runs: the managed heap's numbers are known
	int64_t ManagedBytes = 0;
	int32_t Collections = 0;   // garbage collections since startup
	SceneAudio::Stats Audio;

	static DebugOverlayGame Gather(const GameLoop& loop, GameSystems& systems, const EnvMapVulkanScene& scene, const std::string& sceneName);
};

// The renderer's numbers (EnvMapVulkanProfiler: the last finished frame)
struct DebugOverlayFrame
{
	struct Pass
	{
		std::string Name;
		double GpuMs = 0.0;
	};

	double Fps = 0.0;
	double CpuMs = 0.0;              // the time between frames, smoothed
	bool GpuValid = false;           // the GPU's timestamps are measured
	double GpuMs = 0.0;              // smoothed
	const float* History = nullptr;  // frame times (ms), HistorySize of them, the oldest at HistoryOffset
	uint32_t HistorySize = 0;
	uint32_t HistoryOffset = 0;
	uint32_t Draws = 0;
	uint64_t Triangles = 0;
	uint32_t Meshes = 0;             // of the main view
	uint32_t Culled = 0;
	std::vector<Pass> Passes;        // the frame's top-level passes, smoothed
};

/**
 * The debug overlay (docs/engine/Game_Engine_Runtime_Guide.html, 7.4): what a running game is doing, as a few lines of
 * text and a bar graph of the frame times over the image, top-right. F3 shows and hides it in the player (also in an
 * exported game) and in the editor's viewport.
 *
 * Not UI entities: drawn by the engine after the game's UI, in the HUD's font, at a fixed size in pixels (times the
 * display's content scale, so it reads the same on a Retina display); no scene has it, so a game can't break, hide or save
 * it. It takes no input besides its key. No graphics API here: it adds quads to the frame's UIDrawList, which the renderer
 * draws (EnvMapVulkanUI).
 */
namespace DebugOverlay
{
	// game: nullptr when no game runs (the editor in Edit): only the frame's numbers
	void Build(UIDrawList& list, const glm::vec2& viewSize, float scale, const DebugOverlayFrame& frame, const DebugOverlayGame* game);
}
