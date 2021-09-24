#include "VertexArray.h"

#include "Hazel/Platform/OpenGL/OpenGLVertexArray.h"
#include "Hazel/Renderer/RendererAPI.h"


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
