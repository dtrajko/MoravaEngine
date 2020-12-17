#include "OpenGLVertexArray.h"
#include "../../Renderer/HazelRenderer.h"

#include "../../../Log.h"

#include <GL/glew.h>


namespace Hazel {

	static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return GL_FLOAT;
			case ShaderDataType::Float2:   return GL_FLOAT;
			case ShaderDataType::Float3:   return GL_FLOAT;
			case ShaderDataType::Float4:   return GL_FLOAT;
			case ShaderDataType::Mat3:     return GL_FLOAT;
			case ShaderDataType::Mat4:     return GL_FLOAT;
			case ShaderDataType::Int:      return GL_INT;
			case ShaderDataType::Int2:     return GL_INT;
			case ShaderDataType::Int3:     return GL_INT;
			case ShaderDataType::Int4:     return GL_INT;
			case ShaderDataType::Bool:     return GL_BOOL;
		}

		Log::GetLogger()->error("Unknown ShaderDataType!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		glCreateVertexArrays(1, &m_RendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_RendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		//	Ref<const OpenGLVertexArray> instance = this;
		//	Renderer::Submit([instance]() {
		//		glBindVertexArray(instance->m_RendererID);
		//	});

		glBindVertexArray(m_RendererID);
	}

	void OpenGLVertexArray::Unbind() const
	{
		//	Ref<const OpenGLVertexArray> instance = this;
		//	HazelRenderer::Submit([this]() {
		//		glBindVertexArray(0);
		//	});

		glBindVertexArray(0);
	}

	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		HZ_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

		Bind();
		vertexBuffer->Bind();

		// Ref<OpenGLVertexArray> instance = this;

		// Renderer::Submit([this, vertexBuffer]() {
		const auto& layout = vertexBuffer->GetLayout();
		for (const auto& element : layout)
		{
			auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
			// glEnableVertexAttribArray(instance->m_VertexBufferIndex);
			glEnableVertexAttribArray(m_VertexBufferIndex);
			if (glBaseType == GL_INT)
			{
				// glVertexAttribIPointer(instance->m_VertexBufferIndex,
				glVertexAttribIPointer(m_VertexBufferIndex,
					element.GetComponentCount(),
					glBaseType,
					layout.GetStride(),
					(const void*)(intptr_t)element.Offset);
			}
			else
			{
				// glVertexAttribPointer(instance->m_VertexBufferIndex,
				glVertexAttribPointer(m_VertexBufferIndex,
					element.GetComponentCount(),
					glBaseType,
					element.Normalized ? GL_TRUE : GL_FALSE,
					layout.GetStride(),
					(const void*)(intptr_t)element.Offset);
			}
			// instance->m_VertexBufferIndex++;
			m_VertexBufferIndex++;
		}
		// });
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		Bind();
		indexBuffer->Bind();

		m_IndexBuffer = indexBuffer;
	}

}
