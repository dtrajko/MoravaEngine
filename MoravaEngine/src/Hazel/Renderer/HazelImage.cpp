#include "HazelImage.h"

#include "Hazel/Platform/Vulkan/VulkanImage.h"
#include "Hazel/Platform/OpenGL/OpenGLImage.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<HazelImage2D> HazelImage2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, Buffer buffer)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::RendererAPIType::None: return Ref<HazelImage2D>();
		case RendererAPI::RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, buffer);
		case RendererAPI::RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelImage2D>();
	}

	Ref<HazelImage2D> HazelImage2D::Create(HazelImageFormat format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::RendererAPIType::None: return Ref<HazelImage2D>();
		case RendererAPI::RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(format, width, height, data);
		case RendererAPI::RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(format, width, height);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelImage2D>();
	}

}

