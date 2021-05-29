#include "RenderPass.h"

#include "Hazel/Core/Base.h"
#include "Hazel/Core/Assert.h"
#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/OpenGL/OpenGLRenderPass.h"
#include "Hazel/Platform/Vulkan/VulkanRenderPass.h"

#include "Core/Log.h"


namespace Hazel {

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return Ref<RenderPass>();
		case RendererAPIType::OpenGL:  return Ref<OpenGLRenderPass>::Create(spec);
		case RendererAPIType::Vulkan: return Ref<VulkanRenderPass>::Create(spec);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return Ref<RenderPass>();
	}

}
