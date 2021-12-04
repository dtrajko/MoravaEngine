/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/AssertH2M.h"
#include "H2M/Core/RefH2M.h"
#include "RendererTypesH2M.h"

#include "Core/Log.h"

#include <string>
#include <vector>


namespace H2M
{

	class IndexBufferH2M : public RefCountedH2M
	{
	public:
		virtual ~IndexBufferH2M() {}

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetCount() const = 0;

		virtual uint32_t GetSize() const = 0;
		virtual RendererID_H2M GetRendererID() const = 0;

		static RefH2M<IndexBufferH2M> Create(uint32_t size);
		static RefH2M<IndexBufferH2M> Create(void* data, uint32_t size = 0);

	};

}
