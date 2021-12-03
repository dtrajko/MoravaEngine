/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <chrono>

namespace H2M
{

	class TimerH2M
	{
	public:
		TimerH2M()
		{
			Reset();
		}

		void TimerH2M::Reset()
		{
			m_Start = std::chrono::high_resolution_clock::now();
		}

		float TimerH2M::Elapsed()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count() * 0.001f * 0.001f * 0.001f;
		}

		float TimerH2M::ElapsedMillis()
		{
			return Elapsed() * 1000.0f;
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};

}
