#include "Core/Timer.h"

#include "Core/CommonValues.h"

#include <stdio.h>


Timer* Timer::s_Instance = nullptr;

Timer* Timer::Get()
{
	if (s_Instance == nullptr)
	{
		s_Instance = new Timer();
	}

	return s_Instance;
}

Timer::Timer()
{
	m_TargetUpdateRate = 24.0f;
	m_TargetFPS = 60.0f;
}

Timer::Timer(float targetFPS) : Timer()
{
	m_TargetFPS = targetFPS;
}

Timer::Timer(float targetFPS, float targetUpdateRate) : Timer()
{
	m_TargetUpdateRate = targetUpdateRate;
	m_TargetFPS = targetFPS;
}

void Timer::Update()
{
	m_CurrentTimestamp = (float)glfwGetTime();

	// Render
	m_DeltaTime = m_CurrentTimestamp - m_LastFrameTimestamp;
	m_CanRender = false;
	m_RealFPS = 1.0f / m_DeltaTime;
	if (m_RealFPS <= m_TargetFPS)
	{
		m_CanRender = true;
		m_LastFrameTimestamp = m_CurrentTimestamp;
	}

	// printf("Timer::Update m_RealFPS = %.2ff m_TargetFPS = %.2ff m_DeltaTime = %.2ff m_CanRender = %d\n",
	// 	m_RealFPS, m_TargetFPS, m_DeltaTime, m_CanRender);

	// Update
	m_DeltaTimeUpdate = m_CurrentTimestamp - m_LastUpdateTimestamp;
	m_CanUpdate = false;
	m_RealUpdateRate = 1.0f / m_DeltaTimeUpdate;
	if (m_RealUpdateRate <= m_TargetUpdateRate) {
		m_CanUpdate = true;
		m_LastUpdateTimestamp = m_CurrentTimestamp;
	}

	// printf("Timer::Update m_RealUpdateRate = %.2ff m_TargetUpdateRate = %.2ff m_DeltaTimeUpdate = %.2ff m_CanUpdate = %d\n",
	// 	m_RealUpdateRate, m_TargetUpdateRate, m_DeltaTimeUpdate, m_CanUpdate);
}

Timer::~Timer()
{
}
