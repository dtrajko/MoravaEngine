#include "TextureH2M.h"

#if defined(GRAPHICS_API_OPENGL)
#include "H2M/Platform/OpenGL/OpenGLTextureH2M.h"
#include "H2M/Platform/OpenGL/OpenGLTexture2D_Hazel2D.h"
#endif
#if defined(GRAPHICS_API_VULKAN)
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#endif
#include "RendererAPI_H2M.h"

#include "Core/Util.h"
#if defined(GRAPHICS_API_DIRECTX11)
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11TextureCube.h"
#endif

#include "imgui.h"


namespace H2M
{

	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<Texture2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTexture2D_Hazel2D>::Create(width, height);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTexture2D_H2M>::Create(format, width, height, data);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11Texture2D>::Create(format, width, height, data);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<Texture2D_H2M>();
	}

	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(const std::string& path, bool srgb)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<Texture2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTexture2D_H2M>::Create(path, srgb);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTexture2D_H2M>::Create(path, srgb);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11Texture2D>::Create(path, srgb);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<Texture2D_H2M>();
	}

	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(const std::string& path, TexturePropertiesH2M properties)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None: return RefH2M<Texture2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTexture2D_H2M>::Create(path, properties);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTexture2D_H2M>::Create(path, properties);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11: return RefH2M<DX11Texture2D>::Create(path, properties);
#endif
		}
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Texture2D_H2M>();
	}

	/**** BEGIN Method removed in Vulkan branch ****
	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap)
	{
		switch (RendererAPI_H2M::Current())
		{
		case RendererAPITypeH2M::None:   return RefH2M<Texture2D_H2M>();
#if defined(GRAPHICS_API_OPENGL)
		case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTexture2D>::Create(format, width, height, wrap);
#endif
#if defined(GRAPHICS_API_VULKAN)
		case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTexture2D>::Create(format, width, height, wrap);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
		case RendererAPITypeH2M::DX11:   return RefH2M<DX11Texture2D>::Create(format, width, height, wrap);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<Texture2D_H2M>();
	}
	/**** END Method removed in Vulkan branch ****/

	ImTextureID Texture2D_H2M::GetImTextureID()
	{
		return (ImTextureID)(intptr_t)GetID();
	}

	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTextureCubeH2M>::Create(format, width, height, data);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTextureCubeH2M>::Create(format, width, height, data);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(format, width, height, data);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<TextureCubeH2M>();
	}

	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(const std::string& path)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTextureCubeH2M>::Create(path);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTextureCubeH2M>::Create(path);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(path);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<TextureCubeH2M>();
	}

	// Used only by the OpenGL EnvMap scene. Scheduled for removal.
	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
#if defined(GRAPHICS_API_OPENGL)
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTextureCubeH2M>::Create(format, width, height, true);
#endif
#if defined(GRAPHICS_API_VULKAN)
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTextureCubeH2M>::Create(format, width, height, nullptr);
#endif
#if defined(GRAPHICS_API_DIRECTX11)
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(format, width, height, nullptr);
#endif
		}
		Log::GetLogger()->error("Unknown RendererAPI_H2M");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI_H2M");
		return RefH2M<TextureCubeH2M>();
	}

	void TextureH2M::SetData(void* data, uint32_t size)
	{
	}

	uint32_t TextureH2M::GetBPP(ImageFormatH2M format)
	{
		switch (format)
		{
			case ImageFormatH2M::RGB:    return 3;
			case ImageFormatH2M::RGBA:   return 4;
		}
		return 0;
	}

	/**** BEGIN Removed in Hazel Live 18.03.2021 #2 ****
	uint32_t TextureH2M::CalculateMipMapCount(uint32_t width, uint32_t height)
	{
		uint32_t levels = 1;
		while ((width | height) >> levels)
			levels++;

		return levels;
	}
	/**** END Removed in Hazel Live 18.03.2021 #2 ****/

}
