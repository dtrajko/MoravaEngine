#pragma once

#include "../../Renderer/Buffer.h"
#include "../../Renderer/VertexArray.h"

#include "OpenGLBuffer.h"

#include <memory>
#include <vector>


namespace Hazel {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		void Bind() const;
		void Unbind() const;

		virtual void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) override;

		virtual const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		virtual const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

		virtual uint32_t GetRendererID() const { return m_RendererID; };
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
		std::shared_ptr<IndexBuffer> m_IndexBuffer;
	};

}
