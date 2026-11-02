/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/BufferH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"

#include "Core/Log.h"

#include <stdint.h>
#include <initializer_list>
#include <vector>


namespace H2M {


	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	class OpenGLIndexBufferH2M : public IndexBufferH2M
	{
	public:
		OpenGLIndexBufferH2M(uint32_t size);
		OpenGLIndexBufferH2M(void* data, uint32_t size);
		virtual ~OpenGLIndexBufferH2M();

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0);
		virtual void Bind() const;

		virtual uint32_t GetCount() const { return m_Size / sizeof(uint32_t); }

		virtual uint32_t GetSize() const { return m_Size; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_Size;

		BufferH2M m_LocalData;
	};

}
