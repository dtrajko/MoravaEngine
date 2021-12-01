#include "RenderCommandBufferH2M.h"

#include "H2M/Platform/Vulkan/VulkanRenderCommandBufferH2M.h"

#include "H2M/Renderer/RendererAPI_H2M.h"


namespace H2M
{

	Ref<RenderCommandBufferH2M> RenderCommandBufferH2M::Create(uint32_t count, const std::string& debugName)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return Ref<RenderCommandBufferH2M>();
			case RendererAPITypeH2M::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count, debugName);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RenderCommandBufferH2M>();
	}

	Ref<RenderCommandBufferH2M> RenderCommandBufferH2M::CreateFromSwapChain(const std::string& debugName)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:    return Ref<RenderCommandBufferH2M>();
			case RendererAPITypeH2M::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(debugName, true);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");

		return Ref<RenderCommandBufferH2M>();
	}

}
