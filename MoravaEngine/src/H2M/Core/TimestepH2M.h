/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once


namespace H2M
{

	class TimestepH2M
	{
	public:
		TimestepH2M() {}
		TimestepH2M(float time);

		inline float GetSeconds() const { return m_Time; }
		inline float GetMilliseconds() const { return m_Time * 1000.0f; }

		operator float() { return m_Time; }

	private:
		float m_Time = 0.0f;
	};

}
