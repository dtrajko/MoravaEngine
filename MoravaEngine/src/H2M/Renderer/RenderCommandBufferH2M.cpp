#include "RenderCommandBufferH2M.h"

#include "H2M/Platform/Vulkan/VulkanRenderCommandBufferH2M.h"

#include "H2M/Renderer/RendererAPI_H2M.h"


namespace H2M
{

	RefH2M<RenderCommandBufferH2M> RenderCommandBufferH2M::Create(uint32_t count, const std::string& debugName)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<RenderCommandBufferH2M>();
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanRenderCommandBufferH2M>::Create(count, debugName);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<RenderCommandBufferH2M>();
	}

	RefH2M<RenderCommandBufferH2M> RenderCommandBufferH2M::CreateFromSwapChain(const std::string& debugName)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return RefH2M<RenderCommandBufferH2M>();
			case RendererAPITypeH2M::Vulkan:  return RefH2M<VulkanRenderCommandBufferH2M>::Create(debugName, true);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");

		return RefH2M<RenderCommandBufferH2M>();
	}

}
