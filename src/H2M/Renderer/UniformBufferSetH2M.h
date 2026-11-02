/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */
#pragma once

#include "UniformBufferH2M.h"


namespace H2M
{

	class UniformBufferSetH2M : public RefCountedH2M
	{
	public:
		virtual ~UniformBufferSetH2M() {}

		virtual void Create(uint32_t size, uint32_t binding) = 0;

		virtual RefH2M<UniformBufferH2M> Get(uint32_t binding, uint32_t set = 0, uint32_t frame = 0) = 0;
		virtual void Set(RefH2M<UniformBufferH2M> uniformBuffer, uint32_t set = 0, uint32_t frame = 0) = 0;

		static RefH2M<UniformBufferSetH2M> Create(uint32_t frames);
	};

}
