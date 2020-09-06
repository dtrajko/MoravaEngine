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

		void AddVertexBuffer(OpenGLVertexBuffer* vertexBuffer);
		void SetIndexBuffer(OpenGLIndexBuffer* indexBuffer);

		const std::vector<OpenGLVertexBuffer*>* GetVertexBuffers() const { return &m_VertexBuffers; }
		const OpenGLIndexBuffer* GetIndexBuffer() const { return m_IndexBuffer; }

		virtual uint32_t GetRendererID() const { return m_RendererID; };
	private:
		uint32_t m_RendererID = 0;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<OpenGLVertexBuffer*> m_VertexBuffers;
		OpenGLIndexBuffer* m_IndexBuffer;
	};

}
