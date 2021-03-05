#include "RenderPass.h"

#include "../Core/Base.h"
#include "../Core/Assert.h"
#include "../Platform/OpenGL/OpenGLRenderPass.h"
#include "RendererAPI.h"

#include "Core/Log.h"


namespace Hazel {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPIType::OpenGL:  return Ref<OpenGLRenderPass>::Create(spec);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
