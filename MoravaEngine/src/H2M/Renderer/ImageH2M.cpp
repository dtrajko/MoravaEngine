#include "ImageH2M.h"

#include "H2M/Platform/Vulkan/VulkanImageH2M.h"
#include "H2M/Platform/OpenGL/OpenGLImageH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Platform/DX11/DX11Image.h"


namespace H2M {

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageSpecificationH2M specification, BufferH2M buffer)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D>::Create(specification, buffer);
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D>::Create(specification);
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(specification);
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

	RefH2M<Image2D_H2M> HazelImage2D::Create(ImageSpecification specification, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPIType::None:   return RefH2M<Image2D_H2M>();
			case RendererAPIType::OpenGL: return RefH2M<OpenGLImage2D>::Create(specification, data);
			case RendererAPIType::Vulkan: return RefH2M<VulkanImage2D>::Create(specification);
			case RendererAPIType::DX11:   return RefH2M<DX11Image2D>::Create(specification);
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

}
