/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "H2M/Core/RefH2M.h"
#include "H2M/Renderer/IndexBufferH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"
#include "H2M/Renderer/VertexArrayH2M.h"

#include <memory>
#include <vector>


namespace H2M
{

	class OpenGLVertexArrayH2M : public VertexArrayH2M
	{
	public:
		OpenGLVertexArrayH2M();
		virtual ~OpenGLVertexArrayH2M();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const RefH2M<VertexBufferH2M>& vertexBuffer) override;
		virtual void SetIndexBuffer(const RefH2M<IndexBufferH2M>& indexBuffer) override;

		virtual const std::vector<RefH2M<VertexBufferH2M>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		virtual const RefH2M<IndexBufferH2M>& GetIndexBuffer() const override { return m_IndexBuffer; }

		virtual uint32_t GetRendererID() const override { return m_RendererID; };

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<RefH2M<VertexBufferH2M>> m_VertexBuffers;
		RefH2M<IndexBufferH2M> m_IndexBuffer;
	};

}
