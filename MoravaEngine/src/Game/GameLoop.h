#pragma once

#include <cstdint>


/**
 * The game loop (docs/engine/Game_Engine_Runtime_Guide.html, 1.4), called once per rendered frame while the game runs.
 * Simulation must not depend on the frame time, so it runs in fixed steps of FixedStep seconds: the time that passed is
 * added to an accumulator, and as many whole steps run as it holds (zero, one or several per frame). What is left, as a
 * fraction of a step, is the alpha, for drawing moving bodies between their last two steps. Physics (Phase 3), scripts
 * (Phase 4), audio (Phase 5) and animation (Phase 6) plug in here; until then the loop only keeps time.
 */
class GameLoop
{
public:
	static constexpr float FixedStep = 1.0f / 60.0f;

	// A longer frame (a breakpoint, a hitch, a scene loading) counts as this long: the game slows down for a moment
	// instead of running hundreds of steps to catch up, which make the next frame long too (the "spiral of death")
	static constexpr float MaxFrameTime = 0.25f;

	// One frame: the fixed steps it is due, then the frame's own update. frameTime is real time, in seconds
	void Frame(float frameTime);

	// Back to the start (game time 0, no steps), for the next time the game starts
	void Reset();

	// Game time per real time: 1 normal, 0.5 slow motion, 0 standing still (the loop still runs, UI keeps working)
	void SetTimeScale(float timeScale);
	float GetTimeScale() const { return m_TimeScale; }

	// How far the game is between its last fixed step and the next one, 0 to 1
	float GetAlpha() const { return m_Alpha; }

	uint64_t GetFrameCount() const { return m_FrameCount; }
	uint64_t GetFixedStepCount() const { return m_FixedStepCount; }
	double GetGameTime() const { return m_GameTime; } // in seconds, scaled

private:
	float m_TimeScale = 1.0f;
	float m_Accumulator = 0.0f;
	float m_Alpha = 0.0f;
	uint64_t m_FrameCount = 0;
	uint64_t m_FixedStepCount = 0;
	double m_GameTime = 0.0;
};
