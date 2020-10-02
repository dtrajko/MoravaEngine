#include "RenderPass.h"

#include "../Platform/OpenGL/OpenGLRenderPass.h"


namespace Hazel {

	RenderPass* RenderPass::Create(const RenderPassSpecification& spec)
	{
		return new OpenGLRenderPass(spec);
	}

}
