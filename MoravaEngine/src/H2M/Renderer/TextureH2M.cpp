#include "TextureH2M.h"

#include "H2M/Platform/OpenGL/OpenGLTexture.h"
#include "H2M/Platform/Vulkan/VulkanTexture.h"
#include "H2M/Renderer/RendererAPI.h"

#include "Core/Util.h"
#include "Platform/DX11/DX11Texture2D.h"
#include "Platform/DX11/DX11TextureCube.h"

#include "imgui.h"


namespace H2M {

	H2M::RefH2M<Texture2DH2M> Texture2DH2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data, H2M::TextureProperties properties)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<Texture2DH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLTexture2D>::Create(format, width, height, data);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanTexture2D>::Create(format, width, height, data, properties);
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11Texture2D>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return H2M::RefH2M<Texture2DH2M>();
	}

	H2M::RefH2M<Texture2DH2M> Texture2DH2M::Create(const std::string& path, H2M::TextureProperties properties)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<Texture2DH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLTexture2D>::Create(path, properties);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanTexture2D>::Create(path, properties);
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11Texture2D>::Create(Util::to_wstr(path.c_str()).c_str());
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return H2M::RefH2M<Texture2DH2M>();
	}

	/**** BEGIN Method removed in Vulkan branch ****
	Ref<Texture2DH2M> Texture2DH2M::Create(ImageFormatH2M format, uint32_t width, uint32_t height, TextureWrap wrap)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPIType::None:   return Ref<Texture2DH2M>();
		case RendererAPIType::OpenGL: return Ref<OpenGLTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::Vulkan: return Ref<VulkanTexture2D>::Create(format, width, height, wrap);
		case RendererAPIType::DX11:   return Ref<DX11Texture2D>::Create(format, width, height, wrap);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return Ref<Texture2DH2M>();
	}
	/**** END Method removed in Vulkan branch ****/

	ImTextureID Texture2DH2M::GetImTextureID()
	{
		return (ImTextureID)(intptr_t)GetID();
	}

	H2M::RefH2M<TextureCubeH2M> TextureCubeH2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, const void* data)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLTextureCube>::Create(format, width, height, data);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanTextureCube>::Create(format, width, height, data, H2M::TextureProperties{});
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11TextureCube>::Create(format, width, height, data);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return H2M::RefH2M<TextureCubeH2M>();
	}

	H2M::RefH2M<TextureCubeH2M> TextureCubeH2M::Create(const std::string& path)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLTextureCube>::Create(path);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanTextureCube>::Create(path, H2M::TextureProperties{});
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11TextureCube>::Create(path);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return H2M::RefH2M<TextureCubeH2M>();
	}

	// Used only by the OpenGL EnvMap scene. Scheduled for removal.
	H2M::RefH2M<TextureCubeH2M> TextureCubeH2M::Create(H2M::ImageFormatH2M format, uint32_t width, uint32_t height, bool notUsed)
	{
		switch (H2M::RendererAPI_H2M::Current())
		{
			case H2M::RendererAPITypeH2M::None:   return H2M::RefH2M<TextureCubeH2M>();
			case H2M::RendererAPITypeH2M::OpenGL: return H2M::RefH2M<H2M::OpenGLTextureCube>::Create(format, width, height, true);
			case H2M::RendererAPITypeH2M::Vulkan: return H2M::RefH2M<H2M::VulkanTextureCube>::Create(format, width, height, nullptr, H2M::TextureProperties{});
			case H2M::RendererAPITypeH2M::DX11:   return H2M::RefH2M<DX11TextureCube>::Create(format, width, height, nullptr);
		}
		Log::GetLogger()->error("Unknown RendererAPI");
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return H2M::RefH2M<TextureCubeH2M>();
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
