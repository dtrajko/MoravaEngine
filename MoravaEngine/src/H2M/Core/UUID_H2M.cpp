#include "UUID_H2M.h"

#include <random>


namespace H2M
{

	static std::random_device s_RandomDevice;
	static std::mt19937_64 eng(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	static std::mt19937 eng32(s_RandomDevice());
	static std::uniform_int_distribution<uint32_t> s_UniformDistribution32;

	UUID_H2M::UUID_H2M()
		: m_UUID(s_UniformDistribution(eng))
	{
	}

	UUID_H2M::UUID_H2M(uint64_t uuid)
		: m_UUID(uuid)
	{
	}

	UUID_H2M::UUID_H2M(const UUID_H2M& other)
		: m_UUID(other.m_UUID)
	{
	}


	UUID32_H2M::UUID32_H2M()
		: m_UUID(s_UniformDistribution32(eng32))
	{
	}

	UUID32_H2M::UUID32_H2M(uint32_t uuid)
		: m_UUID(uuid)
	{
	}

	UUID32_H2M::UUID32_H2M(const UUID32_H2M& other)
		: m_UUID(other.m_UUID)
	{
	}

}
