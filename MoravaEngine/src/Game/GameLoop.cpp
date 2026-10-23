#include "GameLoop.h"

#include <algorithm>


void GameLoop::Frame(float frameTime, GameLoopSystems* systems)
{
	const float gameFrameTime = std::clamp(frameTime, 0.0f, MaxFrameTime) * m_TimeScale;
	m_Accumulator += gameFrameTime;

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
	}
	m_Alpha = m_Accumulator / FixedStep;

	// The scripts' OnUpdate(gameFrameTime). Phase 6: animation. Phase 5: the listener and sound positions. Then the
	// scene changes the frame asked for (destroyed entities, a scene to load)
	if (systems)
	{
		systems->Update(gameFrameTime, m_Alpha);
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
	m_GameTime = 0.0;
}

void GameLoop::SetTimeScale(float timeScale)
{
	m_TimeScale = std::max(timeScale, 0.0f);
}
