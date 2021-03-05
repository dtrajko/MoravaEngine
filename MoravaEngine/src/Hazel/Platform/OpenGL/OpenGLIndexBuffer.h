#pragma once

#include "../../Core/Buffer.h"
#include "../../Renderer/IndexBuffer.h"

#include "Core/Log.h"

#include <stdint.h>
#include <initializer_list>
#include <vector>


namespace Hazel {


	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t size);
		OpenGLIndexBuffer(void* data, uint32_t size);
		virtual ~OpenGLIndexBuffer();

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0);
		virtual void Bind() const;

		virtual uint32_t GetCount() const { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const { return m_Size; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_Size;

		Buffer m_LocalData;
	};

}
