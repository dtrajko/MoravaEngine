#pragma once

#include <memory>

#include "../Renderer/Buffer.h"

namespace Hazel {

	class VertexArray
	{
	public:
		virtual ~VertexArray() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) = 0;
		virtual void SetIndexBuffer(IndexBuffer* indexBuffer) = 0;

		virtual const std::vector<VertexBuffer*>& GetVertexBuffers() const = 0;
		virtual const IndexBuffer& GetIndexBuffer() const = 0;

		virtual uint32_t GetID() const = 0;

		static VertexArray* Create();
	};

}
