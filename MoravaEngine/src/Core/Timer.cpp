#include "Core/Timer.h"

#include "Hazel/Renderer/RendererAPI.h"

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

	m_DeltaTime = 0.0f;
	m_LastFrameTimestamp = 0.0f;

	m_StartTimeChrono = std::chrono::high_resolution_clock::now();
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
	switch (Hazel::RendererAPI::Current())
	{
		case Hazel::RendererAPIType::OpenGL:
		case Hazel::RendererAPIType::Vulkan:
		{
			m_CurrentTimestamp = (float)glfwGetTime(); // returns seconds, as a double
			break;
		}
		case Hazel::RendererAPIType::DX11:
		{
			auto elapsedMilliseconds = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - m_StartTimeChrono);
			float elapsedSeconds = (float)(elapsedMilliseconds.count() / 1000.0f);
			m_CurrentTimestamp = elapsedSeconds;
			break;
		}
		default:
		{
			Log::GetLogger()->error("Unknown RendererAPI");
			HZ_CORE_ASSERT(false, "Unknown RendererAPI");
			break;
		}
	}

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
	if (m_RealUpdateRate <= m_TargetUpdateRate)
	{
		m_CanUpdate = true;
		m_LastUpdateTimestamp = m_CurrentTimestamp;
	}

	// printf("Timer::Update m_RealUpdateRate = %.2ff m_TargetUpdateRate = %.2ff m_DeltaTimeUpdate = %.2ff m_CanUpdate = %d\n",
	// 	m_RealUpdateRate, m_TargetUpdateRate, m_DeltaTimeUpdate, m_CanUpdate);
}

Timer::~Timer()
{
}
