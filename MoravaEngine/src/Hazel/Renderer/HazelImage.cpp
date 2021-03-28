#include "HazelImage.h"

#include "Hazel/Platform/Vulkan/VulkanImage.h"
#include "Hazel/Platform/OpenGL/OpenGLImage.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<HazelImage2D> HazelImage2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, Buffer buffer)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, buffer);
		case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<HazelImage2D> HazelImage2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None: return nullptr;
		case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, data);
		case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
