#pragma once

#include "../../Core/Buffer.h"
#include "../../Renderer/Buffer.h"
#include "../../../Log.h"

#include <stdint.h>
#include <initializer_list>
#include <vector>


namespace Hazel {


	//////////////////////////////////////////////////////////////////////////////////
	// VertexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	class OpenGLVertexBuffer
	{
	public:
		OpenGLVertexBuffer(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		OpenGLVertexBuffer(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		virtual ~OpenGLVertexBuffer();

		void SetData(void* data, uint32_t size, uint32_t offset = 0);
		void Bind() const;

		const BufferLayout& GetLayout() const { return m_Layout; }
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

		uint32_t GetSize() const { return m_Size; }
		uint32_t GetRendererID() const { return m_RendererID; }
	private:
		uint32_t m_RendererID = 0;
		unsigned int m_Size;
		VertexBufferUsage m_Usage;
		BufferLayout m_Layout;

		Buffer m_LocalData;
	};

	//////////////////////////////////////////////////////////////////////////////////
	// IndexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	class OpenGLIndexBuffer
	{
	public:
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
