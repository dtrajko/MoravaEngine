/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/RefH2M.h"


namespace H2M
{

	class UniformBufferH2M : public RefCountedH2M
	{
	public:
		virtual ~UniformBufferH2M() {}

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void RT_SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual uint32_t GetBinding() const = 0;

		static RefH2M<UniformBufferH2M> Create(uint32_t size, uint32_t binding);

	};

}
