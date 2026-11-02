#include "GameLoop.h"

#include <algorithm>


void GameLoop::Frame(float frameTime, GameLoopSystems* systems)
{
	m_TimeScale = systems ? std::max(systems->GetTimeScale(), 0.0f) : 1.0f;
	const float realFrameTime = std::clamp(frameTime, 0.0f, MaxFrameTime);
	const float gameFrameTime = realFrameTime * m_TimeScale;
	m_Accumulator += gameFrameTime;

	uint32_t steps = 0;
	while (m_Accumulator >= FixedStep)
	{
		// The systems' fixed step (the input test, the scripts' OnFixedUpdate, the physics step, its contacts and triggers to
		// the scripts); they end it with the input's edges cleared
		if (systems)
		{
			systems->FixedUpdate(FixedStep);
		}
		m_Accumulator -= FixedStep;
		m_FixedStepCount++;
		steps++;
	}

	// Steps per second over each second of real time (a frame's own count jumps between 0, 1 and 2)
	m_RateSteps += steps;
	m_RateTime += realFrameTime;
	if (m_RateTime >= 1.0f)
	{
		m_StepsPerSecond = (float)m_RateSteps / m_RateTime;
		m_RateSteps = 0;
		m_RateTime = 0.0f;
	}
	m_Alpha = m_Accumulator / FixedStep;

	// The scripts' OnUpdate(gameFrameTime). Phase 6: animation. Phase 5: the listener and sound positions. Then the
	// scene changes the frame asked for (destroyed entities, a scene to load)
	if (systems)
	{
		systems->Update(gameFrameTime, realFrameTime, m_Alpha);
	}
	m_GameTime += gameFrameTime;
	m_FrameCount++;
}

void GameLoop::Reset()
{
	m_Accumulator = 0.0f;
	m_Alpha = 0.0f;
	m_FrameCount = 0;
	m_FixedStepCount = 0;
	m_RateSteps = 0;
	m_RateTime = 0.0f;
	m_StepsPerSecond = 0.0f;
	m_GameTime = 0.0;
}
