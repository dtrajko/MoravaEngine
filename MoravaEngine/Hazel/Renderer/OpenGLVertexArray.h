#pragma once

#include "Buffer.h"
#include "OpenGLBuffer.h"

#include <memory>
#include <vector>


namespace Hazel {

	class OpenGLVertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		void Bind() const;
		void Unbind() const;

		void AddVertexBuffer(const std::shared_ptr<OpenGLVertexBuffer>& vertexBuffer);
		void SetIndexBuffer(const std::shared_ptr<OpenGLIndexBuffer>& indexBuffer);

		const std::vector<std::shared_ptr<OpenGLVertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
		const std::shared_ptr<OpenGLIndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

		virtual uint32_t GetRendererID() const { return m_RendererID; };
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<std::shared_ptr<OpenGLVertexBuffer>> m_VertexBuffers;
		std::shared_ptr<OpenGLIndexBuffer> m_IndexBuffer;
	};

}
