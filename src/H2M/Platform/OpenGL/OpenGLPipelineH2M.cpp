/**
 * @package H2M
 * @author  Yan Chernikov (TheCherno)
 * @licence Apache License 2.0
 */

#include "OpenGLPipelineH2M.h"

#include "H2M/Renderer/RendererH2M.h"

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

	OpenGLPipelineH2M::OpenGLPipelineH2M(const PipelineSpecificationH2M& spec)
		: m_Specification(spec)
	{
		Invalidate();
	}

	OpenGLPipelineH2M::~OpenGLPipelineH2M()
	{
		GLuint rendererID = m_VertexArrayRendererID;
		RendererH2M::Submit([rendererID]()
		{
			glDeleteVertexArrays(1, &rendererID);
		});
	}

	void OpenGLPipelineH2M::Invalidate()
	{
		H2M_CORE_ASSERT(m_Specification.Layout.GetElements().size(), "Layout is empty!");

		// Ref<OpenGLPipeline> instance = this;
		// HazelRenderer::Submit([instance]() mutable
		// {
			auto& vertexArrayRendererID = this->m_VertexArrayRendererID;

			if (vertexArrayRendererID)
				glDeleteVertexArrays(1, &vertexArrayRendererID);

			glGenVertexArrays(1, &vertexArrayRendererID);
			glBindVertexArray(vertexArrayRendererID);

#if 0
			const auto& layout = instance->m_Specification.Layout;
			uint32_t attribIndex = 0;
			for (const auto& element : layout)
			{
				auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
				glEnableVertexAttribArray(attribIndex);
				if (glBaseType == GL_INT)
				{
					glVertexAttribIPointer(attribIndex,
						element.GetComponentCount(),
						glBaseType,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				else
				{
					glVertexAttribPointer(attribIndex,
						element.GetComponentCount(),
						glBaseType,
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				attribIndex++;
			}
#endif
			glBindVertexArray(0);
		// });
	}

	void OpenGLPipelineH2M::Bind()
	{
		RefH2M<OpenGLPipelineH2M> instance = this;
		RendererH2M::Submit([instance]()
		{
			glBindVertexArray(instance->m_VertexArrayRendererID);

			const auto& layout = instance->m_Specification.Layout;
			uint32_t attribIndex = 0;
			for (const auto& element : layout)
			{
				auto glBaseType = ShaderDataTypeToOpenGLBaseType(element.Type);
				glEnableVertexAttribArray(attribIndex);
				if (glBaseType == GL_INT)
				{
					glVertexAttribIPointer(attribIndex,
						element.GetComponentCount(),
						glBaseType,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				else
				{
					glVertexAttribPointer(attribIndex,
						element.GetComponentCount(),
						glBaseType,
						element.Normalized ? GL_TRUE : GL_FALSE,
						layout.GetStride(),
						(const void*)(intptr_t)element.Offset);
				}
				attribIndex++;
			}
		});
	}

}
