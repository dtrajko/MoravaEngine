#include "RenderPassH2M.h"

#include "H2M/Core/BaseH2M.h"
#include "H2M/Core/AssertH2M.h"
#include "H2M/Platform/OpenGL/OpenGLRenderPassH2M.h"
#include "H2M/Platform/Vulkan/VulkanRenderPassH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11RenderPass.h"

#include "Core/Log.h"


namespace H2M
{

	RefH2M<RenderPassH2M> RenderPassH2M::Create(const RenderPassSpecificationH2M& spec)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:    H2M_CORE_ASSERT(false, "RendererAPI_H2M::None is currently not supported!"); return RefH2M<RenderPassH2M>();
		case RendererAPITypeH2M::OpenGL:  return RefH2M<OpenGLRenderPassH2M>::Create(spec);
		case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanRenderPassH2M>::Create(spec);
		case RendererAPITypeH2M::DX11:    return RefH2M<DX11RenderPass>::Create(spec);
		}

		Log::GetLogger()->error("Unknown RendererAPI!");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI!");
		return RefH2M<RenderPassH2M>();
	}

}
