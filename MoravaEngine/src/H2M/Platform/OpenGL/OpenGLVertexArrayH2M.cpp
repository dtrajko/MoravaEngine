/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLVertexArrayH2M.h"

#include "H2M/Platform/OpenGL/OpenGLVertexBufferH2M.h"
#include "H2M/Renderer/RendererH2M.h"

#include "Core/Log.h"

#include <GL/glew.h>


namespace H2M
{

	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataTypeH2M type)
	{
		switch (type)
		{
			case ShaderDataTypeH2M::Float:    return GL_FLOAT;
			case ShaderDataTypeH2M::Float2:   return GL_FLOAT;
			case ShaderDataTypeH2M::Float3:   return GL_FLOAT;
			case ShaderDataTypeH2M::Float4:   return GL_FLOAT;
			case ShaderDataTypeH2M::Mat3:     return GL_FLOAT;
			case ShaderDataTypeH2M::Mat4:     return GL_FLOAT;
			case ShaderDataTypeH2M::Int:      return GL_INT;
			case ShaderDataTypeH2M::Int2:     return GL_INT;
			case ShaderDataTypeH2M::Int3:     return GL_INT;
			case ShaderDataTypeH2M::Int4:     return GL_INT;
			case ShaderDataTypeH2M::Bool:     return GL_BOOL;
		}

		Log::GetLogger()->error("Unknown ShaderDataType!");
		return 0;
	}

	OpenGLVertexArrayH2M::OpenGLVertexArrayH2M()
	{
		RendererH2M::Submit([this]() {
			glCreateVertexArrays(1, &m_RendererID);
		});
	}

	OpenGLVertexArrayH2M::~OpenGLVertexArrayH2M()
	{
		// GLuint rendererID = m_RendererID;
		// RendererH2M::Submit([rendererID]() {});
		{
			glDeleteVertexArrays(1, &m_RendererID);
		}
	}

	void OpenGLVertexArrayH2M::Bind() const
	{
		// RefH2M<const OpenGLVertexArrayH2M> instance = this;
		// RendererH2M::Submit([instance]() {});
		{
			glBindVertexArray(m_RendererID);
		}
	}

	void OpenGLVertexArrayH2M::Unbind() const
	{
		// RefH2M<const OpenGLVertexArrayH2M> instance = this;
		// RendererH2M::Submit([this]() {});
		{
			glBindVertexArray(0);
		}
	}

	void OpenGLVertexArrayH2M::AddVertexBuffer(const RefH2M<VertexBufferH2M>& vertexBuffer)
	{
		H2M_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

		Bind();
		vertexBuffer->Bind();

		RefH2M<OpenGLVertexArrayH2M> instance = this;
		RendererH2M::Submit([instance, vertexBuffer]() mutable {
			const auto& layout = vertexBuffer->GetLayout();
			for (const auto& element : layout)
			{
				auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
				glEnableVertexAttribArray(instance->m_VertexBufferIndex);
				if (glBaseType == GL_INT)
				{
					glVertexAttribIPointer(instance->m_VertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				else
				{
					glVertexAttribPointer(instance->m_VertexBufferIndex,
						element.GetComponentCount(),
						glBaseType,
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				instance->m_VertexBufferIndex++;
			}
		});
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArrayH2M::SetIndexBuffer(const RefH2M<IndexBufferH2M>& indexBuffer)
	{
		Bind();
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}

}
