// Render textures in the editor (docs/engine/Game_Engine_Runtime_Guide.html, 7.5): a camera's Target in the Inspector
// (choose, drop or make a .mrtex file, its size and format), and a changed render texture applied at the start of the
// next frame: its new texture into every material map and UI image that shows it.

#include "EnvMapVulkan/EnvMapVulkanMaterialLibrary.h"
#include "EnvMapVulkan/EnvMapVulkanRenderTexture.h"
#include "EnvMapVulkan/EnvMapVulkanRenderer.h"

#include "Core/FileDialogs.h"
#include "Core/Log.h"

#include "Project/Project.h"

#include "UI/UILayout.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>


#include "EditorState.h"


// Settings to write to a render texture's file, at the start of the next frame (a new texture: the device must be idle)
struct PendingRenderTextureChange
{
	std::string FilePath;
	RenderTextureSettings Settings;
};
static std::vector<PendingRenderTextureChange> s_PendingRenderTextureChanges;

void ProcessRenderTextureChanges()
{
	if (s_PendingRenderTextureChanges.empty())
	{
		return;
	}
	const std::vector<PendingRenderTextureChange> changes = std::move(s_PendingRenderTextureChanges);
	s_PendingRenderTextureChanges.clear();
	EnvMapVulkanRenderer::WaitIdle(); // the old texture may be read by frames in flight
	for (const PendingRenderTextureChange& change : changes)
	{
		std::string error;
		if (!EnvMapVulkanRenderTexture::Save(change.FilePath, change.Settings, error))
		{
			ShowNotification("Render texture not saved: " + error, true);
			continue;
		}
		const std::shared_ptr<EnvMapVulkanRenderTexture> renderTexture = EnvMapVulkanRenderTexture::Get(change.FilePath);
		if (!renderTexture)
		{
			continue;
		}

		// The materials that show it get its new texture (they hold the old one)
		uint32_t maps = 0;
		for (H2M::RefH2M<EnvMapVulkanMaterial> material : EnvMapVulkanMaterialLibrary::GetMaterials())
		{
			for (uint32_t slot = 0; slot < EnvMapVulkanMaterial::MapCount; slot++)
			{
				const H2M::RefH2M<H2M::Texture2D_H2M> map = material->GetMap(slot);
				if (map && map != renderTexture->GetTexture() && map->GetPath() == renderTexture->GetFilePath())
				{
					material->SetMap(slot, renderTexture->GetTexture());
					maps++;
				}
			}
		}
		UILayout::ClearImageCache(); // UI images look their textures up again
		const RenderTextureSettings& settings = renderTexture->GetSettings();
		Log::GetLogger()->info("Render texture '{0}': {1}x{2}, {3} ({4} material map(s) updated)", ProjectPaths::ToStored(change.FilePath),
			settings.Width, settings.Height, RenderTextureSettings::GetFormatName(settings.Format), maps);
	}
}

// A new render texture file, chosen with a save dialog (in the project's assets), with the default settings: its path, or
// empty when cancelled or not written
static std::string CreateRenderTextureFile()
{
	std::string filepath = FileDialogs::SaveFile({ { "Render Texture", "mrtex" } }, GetAssetFolderForDialog());
	if (filepath.empty())
	{
		return std::string();
	}
	std::filesystem::path path = std::filesystem::u8path(filepath);
	if (!EnvMapVulkanRenderTexture::IsRenderTextureFile(filepath))
	{
		path += EnvMapVulkanRenderTexture::FileExtension;
	}
	std::string error;
	if (!EnvMapVulkanRenderTexture::Save(path.u8string(), RenderTextureSettings(), error))
	{
		ShowNotification("Render texture not created: " + error, true);
		return std::string();
	}
	return path.lexically_normal().generic_u8string();
}

void RenderTargetControls(CameraComponent& camera)
{
	ImGui::SeparatorText("Target");
	FileControls("CameraTarget", "Target", camera.Target, "(the screen)", "Render Texture", "mrtex", { EnvMapVulkanRenderTexture::FileExtension },
		"Where the camera draws: the screen, or a render texture (.mrtex) that a material's map or a UI image shows\n"
		"(a monitor, a mirror, a minimap). A camera with a target never shows the game. Drop one from the Content\n"
		"Browser here, choose one with ..., or make one with New.");
	if (ImGui::Button("New Render Texture..."))
	{
		const std::string filepath = CreateRenderTextureFile();
		if (!filepath.empty())
		{
			camera.Target = filepath;
		}
	}
	if (camera.Target.empty())
	{
		return;
	}

	const std::shared_ptr<EnvMapVulkanRenderTexture> renderTexture = EnvMapVulkanRenderTexture::Get(camera.Target);
	if (!renderTexture)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.4f, 1.0f), "The file can't be read (the Console says why)");
		return;
	}

	// The render texture's own settings, written to its file (every camera and material that uses the file sees them)
	RenderTextureSettings settings = renderTexture->GetSettings();
	bool apply = false;
	// The size while it is dragged or typed: applied (a new texture) when the field is let go
	static std::string s_SizeFile;
	static bool s_SizeEditing = false;
	static int s_Size[2] = {};
	if (!s_SizeEditing || s_SizeFile != renderTexture->GetFilePath())
	{
		s_Size[0] = (int)settings.Width;
		s_Size[1] = (int)settings.Height;
	}
	ImGui::DragInt2("Size", s_Size, 4.0f, (int)RenderTextureSettings::MinSize, (int)RenderTextureSettings::MaxSize, "%d px", ImGuiSliderFlags_AlwaysClamp);
	s_SizeEditing = ImGui::IsItemActive();
	s_SizeFile = renderTexture->GetFilePath();
	apply |= ImGui::IsItemDeactivatedAfterEdit();
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The render texture's width and height in pixels (saved to its file). Its aspect ratio is the\n"
			"camera's: a square texture for a square screen. More pixels: sharper, and slower to draw.");
	}
	settings.Width = (uint32_t)s_Size[0];
	settings.Height = (uint32_t)s_Size[1];
	int format = (int)settings.Format;
	if (ImGui::Combo("Format", &format, "Color\0HDR\0"))
	{
		settings.Format = (RenderTextureFormat)format;
		apply = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Color: the camera's finished picture, exposed and tonemapped by the camera itself, as a screen\n"
			"shows it (a monitor, a minimap).\n"
			"HDR: its light before tonemapping, exposed with the view that sees it (a mirror, which should look as bright\n"
			"as what it reflects).");
	}
	if (apply && settings != renderTexture->GetSettings())
	{
		s_PendingRenderTextureChanges.push_back({ renderTexture->GetFilePath(), settings });
	}

	int drawEvery = (int)camera.DrawEvery;
	if (ImGui::DragInt("Draw Every", &drawEvery, 0.1f, 1, 120, drawEvery == 1 ? "frame" : "%d frames", ImGuiSliderFlags_AlwaysClamp))
	{
		camera.DrawEvery = (uint32_t)drawEvery;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How often the camera draws its picture: every frame, or every few (a security monitor doesn't\n"
			"need 60 pictures a second, and each one costs about as much as a small view of the scene).");
	}
	ImGui::Checkbox("Enabled", &camera.Enabled);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The camera draws into its texture. Off: the texture keeps its last picture and the camera costs\n"
			"nothing (a monitor nobody looks at; a script turns it on and off with Camera.Enabled).");
	}
}
