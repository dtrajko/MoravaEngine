#include "ImageH2M.h"

#include "H2M/Platform/Vulkan/VulkanImageH2M.h"
#include "H2M/Platform/OpenGL/OpenGLImageH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"
#include "Platform/DX11/DX11Image.h"


namespace H2M
{

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, BufferH2M buffer)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D_H2M>::Create(format, width, height, buffer);
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D_H2M>::Create(format, width, height);
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(format, width, height);
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

	RefH2M<Image2D_H2M> Image2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Image2D_H2M>();
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLImage2D_H2M>::Create(format, width, height, data);
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanImage2D_H2M>::Create(format, width, height);
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Image2D>::Create(format, width, height);
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Image2D_H2M>();
	}

}
