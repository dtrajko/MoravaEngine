#pragma once

#include "H2M/Core/BufferH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"

#include "Core/Log.h"

#include <stdint.h>
#include <initializer_list>
#include <vector>


namespace H2M {


	//////////////////////////////////////////////////////////////////////////////////
	// VertexBuffer
	//////////////////////////////////////////////////////////////////////////////////

	class OpenGLVertexBufferH2M : public VertexBufferH2M
	{
	public:
		OpenGLVertexBufferH2M(void* data, uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Static);
		OpenGLVertexBufferH2M(uint32_t size, VertexBufferUsage usage = VertexBufferUsage::Dynamic);
		virtual ~OpenGLVertexBufferH2M();

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0);
		virtual void Bind() const;
		virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;

		virtual const VertexBufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const VertexBufferLayout& layout) override { m_Layout = layout; }

		virtual uint32_t GetSize() const { return m_Size; }
		virtual RendererID GetRendererID() const { return m_RendererID; }

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_Size;
		VertexBufferUsage m_Usage;
		VertexBufferLayout m_Layout;

		Buffer m_LocalData;
	};

}
