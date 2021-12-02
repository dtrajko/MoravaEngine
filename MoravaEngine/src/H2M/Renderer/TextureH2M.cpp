#include "TextureH2M.h"

#include "H2M/Platform/OpenGL/OpenGLTextureH2M.h"
#include "H2M/Platform/Vulkan/VulkanTextureH2M.h"
#include "H2M/Renderer/RendererAPI_H2M.h"

#include "Core/Util.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11TextureCube.h"

#include "imgui.h"


namespace H2M
{

	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, TexturePropertiesH2M properties)
	{
		switch (RendererAPI_H2M::Current())
		{
			case RendererAPITypeH2M::None:   return RefH2M<Texture2D_H2M>();
			case RendererAPITypeH2M::OpenGL: return RefH2M<OpenGLTexture2D_H2M>::Create(format, width, height, data);
			case RendererAPITypeH2M::Vulkan: return RefH2M<VulkanTexture2D_H2M>::Create(format, width, height, data, properties);
			case RendererAPITypeH2M::DX11:   return RefH2M<DX11Texture2D>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Texture2D_H2M>();
	}

	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(const std::string& path, TexturePropertiesH2M properties)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<Texture2D_H2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return RefH2M<H2M::OpenGLTexture2D>::Create(path, properties);
			case H2M::RendererAPITypeH2M::Vulkan: return RefH2M<H2M::VulkanTexture2D>::Create(path, properties);
			case H2M::RendererAPITypeH2M::DX11:   return RefH2M<DX11Texture2D>::Create(Util::to_wstr(path.c_str()).c_str());
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Texture2D_H2M>();
	}

	/**** BEGIN Method removed in Vulkan branch ****
	RefH2M<Texture2D_H2M> Texture2D_H2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:   return RefH2M<Texture2D_H2M>();
		case RendererAPIType::OpenGL: return RefH2M<OpenGLTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::Vulkan: return RefH2M<VulkanTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::DX11:   return RefH2M<DX11Texture2D>::Create(format, width, height, wrap);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<Texture2D_H2M>();
	}
	/**** END Method removed in Vulkan branch ****/

	ImTextureID Texture2D_H2M::GetImTextureID()
	{
		return (ImTextureID)(intptr_t)GetID();
	}

	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return RefH2M<H2M::OpenGLTextureCube>::Create(format, width, height, data);
			case H2M::RendererAPITypeH2M::Vulkan: return RefH2M<H2M::VulkanTextureCube>::Create(format, width, height, data, TexturePropertiesH2M{});
			case H2M::RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<TextureCubeH2M>();
	}

	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(const std::string& path)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return RefH2M<H2M::OpenGLTextureCubeH2M>::Create(path);
			case H2M::RendererAPITypeH2M::Vulkan: return RefH2M<H2M::VulkanTextureCubeH2M>::Create(path, TexturePropertiesH2M{});
			case H2M::RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(path);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<TextureCubeH2M>();
	}

	// Used only by the OpenGL EnvMap scene. Scheduled for removal.
	RefH2M<TextureCubeH2M> TextureCubeH2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return RefH2M<H2M::OpenGLTextureCubeH2M>::Create(format, width, height, true);
			case H2M::RendererAPITypeH2M::Vulkan: return RefH2M<H2M::VulkanTextureCubeH2M>::Create(format, width, height, nullptr, TexturePropertiesH2M{});
			case H2M::RendererAPITypeH2M::DX11:   return RefH2M<DX11TextureCube>::Create(format, width, height, nullptr);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		H2M_CORE_ASSERT(false, "Unknown RendererAPI");
		return RefH2M<TextureCubeH2M>();
	}

	void TextureH2M::SetData(void* data, uint32_t size)
	{
	}

	uint32_t TextureH2M::GetBPP(H2M::ImageFormatH2M format)
	{
		switch (format)
		{
			case H2M::ImageFormatH2M::RGB:    return 3;
			case H2M::ImageFormatH2M::RGBA:   return 4;
		}
		return 0;
	}

}
