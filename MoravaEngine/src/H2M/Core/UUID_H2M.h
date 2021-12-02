#pragma once

#include "BaseH2M.h"

#include <xhash>


namespace H2M {

	// "UUID" (universally unique identifier) or GUID is (usually) a 128-bit integer
	// used to "uniquely" identify information. In Hazel, even though we use the term
	// GUID and UUID, at the moment we're simply using a randomly generated 64-bit
	// integer, as the possibility of a clash is low enough for now.
	// This may change in the future.
	class UUID_H2M
	{
	public:
		UUID_H2M();
		UUID_H2M(uint64_t uuid);
		UUID_H2M(const UUID_H2M& other);

		operator uint64_t () { return m_UUID; }
		operator const uint64_t () const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

	class UUID32_H2M
	{
	public:
		UUID32_H2M();
		UUID32_H2M(uint32_t uuid);
		UUID32_H2M(const UUID32_H2M& other);

		operator uint32_t () { return m_UUID; }
		operator const uint32_t() const { return m_UUID; }
	private:
		uint32_t m_UUID;
	};
		
}

namespace std {

	template <>
	struct hash<H2M::UUID_H2M>
	{
		std::size_t operator()(const H2M::UUID_H2M& uuid) const
		{
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};

	template <>
	struct hash<H2M::UUID32_H2M>
	{
		std::size_t operator()(const H2M::UUID32_H2M& uuid) const
		{
			return hash<uint32_t>()((uint32_t)uuid);
		}
	};
}
