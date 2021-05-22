#define _CRT_SECURE_NO_WARNINGS

#pragma once

#include "Hazel/Core/Ref.h"
#include "Hazel/Renderer/IndexBuffer.h"
#include "Hazel/Renderer/VertexBuffer.h"
#include "Hazel/Renderer/VertexArray.h"

#include <memory>
#include <vector>


namespace Hazel {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
		virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

		virtual RendererID GetRendererID() const override { return m_RendererID; };

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_VertexBufferIndex = 0;
		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer;
	};

}
