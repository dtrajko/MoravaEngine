#pragma once

#include "H2M/Core/AssertH2M.h"
#include "H2M/Core/BaseH2M.h"

#include "Core/Log.h"

#include <stdint.h>
#include <memory>


namespace H2M
{

	struct BufferH2M
	{
		void* Data;
		uint32_t Size;

		BufferH2M()
			: Data(nullptr), Size(0)
		{
		}

		BufferH2M(void* data, uint32_t size)
			: Data(data), Size(size)
		{
		}

		static BufferH2M Copy(const void* data, uint32_t size)
		{
			BufferH2M buffer;
			buffer.Allocate(size);
			memcpy(buffer.Data, data, size);
			return buffer;
		}

		void Allocate(uint32_t size)
		{
			// delete[] Data;
			Data = nullptr;

			if (size == 0)
				return;

			Data = new byte[size];
			Size = size;
		}

		void Release()
		{
			delete[] Data;
			Data = nullptr;
			Size = 0;
		}

		void ZeroInitialize()
		{
			if (Data)
			{
				memset(Data, 0, Size);
			}
		}

		template<typename T>
		T& Read(uint32_t offset = 0)
		{
			return *(T*)((byte*)Data + offset);
		}

		byte* ReadBytes(uint32_t size, uint32_t offset)
		{
			HZ_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
			byte* buffer = new byte[size];
			memcpy(buffer, (byte*)Data + offset, size);
			return buffer;
		}

		void Write(void* data, uint32_t size, uint32_t offset = 0)
		{
			HZ_CORE_ASSERT(offset + size <= Size, "Buffer overflow!");
			memcpy((byte*)Data + offset, data, size);
		}

		operator bool() const
		{
			return Data;
		}

		byte& operator[](int index)
		{
			return ((byte*)Data)[index];
		}

		byte operator[](int index) const
		{
			return ((byte*)Data)[index];
		}

		template<typename T>
		T* As()
		{
			return (T*)Data;
		}

		inline uint32_t GetSize() const { return Size; }
	};

}
