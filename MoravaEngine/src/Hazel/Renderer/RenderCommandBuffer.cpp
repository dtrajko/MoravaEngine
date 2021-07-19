#include "RenderCommandBuffer.h"

#include "Hazel/Platform/Vulkan/VulkanRenderCommandBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(uint32_t count, const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return Ref<RenderCommandBuffer>();
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count, debugName);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RenderCommandBuffer>();
	}

	Ref<RenderCommandBuffer> RenderCommandBuffer::CreateFromSwapChain(const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:    return Ref<RenderCommandBuffer>();
			case RendererAPIType::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(debugName, true);
		}

		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<RenderCommandBuffer>();
	}

}
