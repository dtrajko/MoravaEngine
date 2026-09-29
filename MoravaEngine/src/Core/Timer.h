#pragma once

#include <chrono>


/**
 * A singleton class
 * 
 */
class Timer
{
public:
	Timer();
	Timer(float targetFPS);
	Timer(float targetFPS, float targetUpdateRate);
	static Timer* Get();
	inline void SetTargetFPS(float targetFPS) { m_TargetFPS = targetFPS; };
	inline void SetTargetUpdateRate(float targetUpdateRate) { m_TargetUpdateRate = targetUpdateRate; };
	inline const float GetRealFPS() const { return m_RealFPS; };
	inline const float GetRealUpdateRate() const { return m_RealUpdateRate; };
	inline const float GetDeltaTime() const { return m_DeltaTime; };
	inline float GetCurrentTimestamp() { return m_CurrentTimestamp; };
	void Update();
	bool CanRender() { return m_CanRender; };
	bool CanUpdate() { return m_CanUpdate; };
	~Timer();

private:
	static Timer* s_Instance;

	float m_CurrentTimestamp = 0.0f; // time in seconds (initialized: it was read before the first Update(), giving garbage like -4.3e8)

	// Render
	float m_TargetFPS = 0.0f;
	float m_RealFPS = 0.0f;
	float m_LastFrameTimestamp = 0.0f;
	float m_DeltaTime = 0.0f;
	bool m_CanRender = false;

	// Update
	float m_TargetUpdateRate = 0.0f;
	float m_RealUpdateRate = 0.0f;
	float m_LastUpdateTimestamp = 0.0f;
	float m_DeltaTimeUpdate = 0.0f;
	bool m_CanUpdate = false;

	// Using chrono for DirectX 11 as GLFW is not available for it
#ifdef _WIN32
	std::chrono::time_point<std::chrono::steady_clock> m_StartTimeChrono;
#else
	std::chrono::time_point<std::chrono::system_clock> m_StartTimeChrono;
#endif

};
