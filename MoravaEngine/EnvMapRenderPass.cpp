#include "EnvMapRenderPass.h"

#include "Hazel/Core/Assert.h"
#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Renderer/RendererAPI.h"
#include "Hazel/Renderer/RenderPass.h"

#include "Log.h"


Hazel::Ref<EnvMapRenderPass> EnvMapRenderPass::Create(const Hazel::RenderPassSpecification& spec)
{
	return Hazel::Ref<Hazel::OpenGLRenderPass>::Create(spec);
}
