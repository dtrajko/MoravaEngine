#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Assert.h"
#include "Hazel/Core/Ref.h"
#include "RendererTypes.h"

#include "Core/Log.h"

#include <string>
#include <vector>


namespace Hazel {

	class IndexBuffer : public RefCounted
	{
	public:
		virtual ~IndexBuffer() {}

		virtual void SetData(void* buffer, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind() const = 0;

		virtual uint32_t GetCount() const = 0;

		virtual uint32_t GetSize() const = 0;
		virtual RendererID GetRendererID() const = 0;

		static Ref<IndexBuffer> Create(uint32_t size);
		static Ref<IndexBuffer> Create(void* data, uint32_t size = 0);

	};

}
