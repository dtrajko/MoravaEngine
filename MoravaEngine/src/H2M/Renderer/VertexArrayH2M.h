/**
 * @package H2M (Hazel to Morava)
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#pragma once

#include <memory>

#include "H2M/Renderer/IndexBufferH2M.h"
#include "H2M/Renderer/VertexBufferH2M.h"


namespace H2M
{

	class VertexArrayH2M : public RefCountedH2M
	{
	public:
		virtual ~VertexArrayH2M() {}

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void AddVertexBuffer(const RefH2M<VertexBufferH2M>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const RefH2M<IndexBufferH2M>& indexBuffer) = 0;

		virtual const std::vector<RefH2M<VertexBufferH2M>>& GetVertexBuffers() const = 0;
		virtual const RefH2M<IndexBufferH2M>& GetIndexBuffer() const = 0;

		virtual RendererID_H2M GetRendererID() const = 0;

		static RefH2M<VertexArrayH2M> Create();

	};

}
