#include "ImGui.h"

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Platform/Vulkan/VulkanTexture.h"
#include "Hazel/Platform/Vulkan/VulkanImage.h"

#include "Hazel/Platform/OpenGL/OpenGLTexture.h"
#include "Hazel/Platform/OpenGL/OpenGLImage.h"

#include "backends/imgui_impl_vulkan_with_textures.h"


namespace ImGui
{
	// extern bool ImageButtonEx(ImGuiID id, ImTextureID texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec2& padding, const ImVec4& bg_col, const ImVec4& tint_col);
}

namespace Hazel::UI {

	void Image(const Ref<HazelImage2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
			const auto& imageInfo = vulkanImage->GetImageInfo();
			if (!imageInfo.ImageView)
				return;
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void Image(const Ref<HazelImage2D>& image, uint32_t imageLayer, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			ImGui::Image((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
			auto imageInfo = vulkanImage->GetImageInfo();
			imageInfo.ImageView = vulkanImage->GetLayerImageView(imageLayer);
			if (!imageInfo.ImageView)
				return;
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void ImageMip(const Ref<HazelImage2D>& image, uint32_t mip, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
		auto imageInfo = vulkanImage->GetImageInfo();
		imageInfo.ImageView = vulkanImage->GetMipImageView(mip);
		if (!imageInfo.ImageView)
			return;

		const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
		ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
	}

	void Image(const Ref<HazelTexture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<OpenGLImage2D> image = texture->GetImage().As<OpenGLImage2D>();
			ImGui::Image((ImTextureID)(size_t)image->GetRendererID(), size, uv0, uv1, tint_col, border_col);
		}
		else
		{
			Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
			const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
			if (!imageInfo.imageView)
				return;
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	bool ImageButton(const Ref<HazelImage2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, image, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<HazelImage2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<OpenGLImage2D> glImage = image.As<OpenGLImage2D>();
			return ImGui::ImageButton((ImTextureID)(size_t)glImage->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
			const auto& imageInfo = vulkanImage->GetImageInfo();
			if (!imageInfo.ImageView)
			{
				return false;
			}

			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptor().imageLayout);
			ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.ImageView) >> 32) ^ (uint32_t)imageInfo.ImageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				id = id ^ strID;
			}
			// return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, ImVec2{ (float)frame_padding, (float)frame_padding }, bg_col, tint_col);

			return false;
		}
	}

	bool ImageButton(const Ref<HazelTexture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		return ImageButton(nullptr, texture, size, uv0, uv1, frame_padding, bg_col, tint_col);
	}

	bool ImageButton(const char* stringID, const Ref<HazelTexture2D>& texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
	{
		// HZ_CORE_VERIFY(texture);
		if (!texture)
			return false;

		if (RendererAPI::Current() == RendererAPIType::OpenGL)
		{
			Ref<OpenGLImage2D> image = texture->GetImage().As<OpenGLImage2D>();
			return ImGui::ImageButton((ImTextureID)(size_t)image->GetRendererID(), size, uv0, uv1, frame_padding, bg_col, tint_col);
		}
		else
		{
			Ref<VulkanTexture2D> vulkanTexture = texture.As<VulkanTexture2D>();
			
			// This is technically okay, could mean that GPU just hasn't created the texture yet
			// HZ_CORE_VERIFY(vulkanTexture->GetImage()); 
			if (!vulkanTexture->GetImage())
				return false;

			const VkDescriptorImageInfo& imageInfo = vulkanTexture->GetVulkanDescriptorInfo();
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.sampler, imageInfo.imageView, imageInfo.imageLayout);
			ImGuiID id = (ImGuiID)((((uint64_t)imageInfo.imageView) >> 32) ^ (uint32_t)imageInfo.imageView);
			if (stringID)
			{
				const ImGuiID strID = ImGui::GetID(stringID);
				id = id ^ strID;
			}
			// return ImGui::ImageButtonEx(id, textureID, size, uv0, uv1, ImVec2{ (float)frame_padding, (float)frame_padding }, bg_col, tint_col);
			return false;
		}
	}

}
