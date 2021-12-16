/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

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
		OpenGLVertexBufferH2M(void* data, uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Static);
		OpenGLVertexBufferH2M(uint32_t size, VertexBufferUsageH2M usage = VertexBufferUsageH2M::Dynamic);
		virtual ~OpenGLVertexBufferH2M();

		virtual void SetData(void* data, uint32_t size, uint32_t offset = 0);
		virtual void Bind() const;
		virtual void RT_SetData(void* buffer, uint32_t size, uint32_t offset = 0) override;

		virtual const VertexBufferLayoutH2M& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const VertexBufferLayoutH2M& layout) override { m_Layout = layout; }

		virtual uint32_t GetSize() const { return m_Size; }
		virtual uint32_t GetRendererID() const { return m_RendererID; }

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_Size;
		VertexBufferUsageH2M m_Usage;
		VertexBufferLayoutH2M m_Layout;

		BufferH2M m_LocalData;
	};

}
