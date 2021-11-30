#include "HazelImage.h"

#include "H2M/Platform/Vulkan/VulkanImage.h"
#include "H2M/Platform/OpenGL/OpenGLImage.h"
#include "Platform/DX11/DX11Image.h"

#include "H2M/Renderer/RendererAPI.h"

namespace H2M {

	Ref<HazelImage2D> HazelImage2D::Create(ImageSpecification specification, Buffer buffer)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return Ref<HazelImage2D>();
			case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(specification, buffer);
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(specification);
			case RendererAPIType::DX11:   return Ref<DX11Image2D>::Create(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelImage2D>();
	}

	Ref<HazelImage2D> HazelImage2D::Create(ImageSpecification specification, const void* data)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPIType::None:   return Ref<HazelImage2D>();
			case RendererAPIType::OpenGL: return Ref<OpenGLImage2D>::Create(specification, data);
			case RendererAPIType::Vulkan: return Ref<VulkanImage2D>::Create(specification);
			case RendererAPIType::DX11:   return Ref<DX11Image2D>::Create(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<HazelImage2D>();
	}

}
