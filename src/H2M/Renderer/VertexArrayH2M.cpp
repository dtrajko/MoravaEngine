#include "VertexArrayH2M.h"

#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLVertexArrayH2M.h"
#endif
#include "H2M/Renderer/RendererAPI_H2M.h"


namespace H2M
{

	RefH2M<VertexArrayH2M> VertexArrayH2M::Create()
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    H2M_CORE_ASSERT(false, "RendererAPI_H2M::None is currently not supported!"); return RefH2M<VertexArrayH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLVertexArrayH2M>::Create();
#endif
			// case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanVertexArrayH2M>::Create();
			// case RendererAPITypeH2M::DX11:    return RefH2M<DX11VertexArrayH2M>::Create();
			default: break;
		}

		// H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		Log::GetLogger()->error("Unknown RendererAPI");
		return RefH2M<VertexArrayH2M>();
	}

}
