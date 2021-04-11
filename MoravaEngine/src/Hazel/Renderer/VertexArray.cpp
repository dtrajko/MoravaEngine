#include "VertexArray.h"

#include "../Platform/OpenGL/OpenGLVertexArray.h"


namespace Hazel {

	Ref<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return Ref<VertexArray>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLVertexArray>::Create();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<VertexArray>();
	}

}
