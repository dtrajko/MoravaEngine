// The Assets panel (docs/engine/Game_Engine_Runtime_Guide.html, 10.1 and the debug view of 10.3), and keeping the registry
// current: files moved in Explorer or by Git are seen when the editor gets the focus back.


#include "Assets/ModelCook.h"
#include "Core/ResourceManager.h"
#include "H2M/Renderer/ModelH2M.h"
#include "Assets/TextureCook.h"
#include "Core/Application.h"
#include "Core/Log.h"
#include "Project/AssetRegistry.h"
#include "Project/Project.h"

#include "imgui.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>


#include "EditorState.h"


bool s_ShowAssetsPanel = false;

void UpdateAssetRegistry()
{
	static bool s_WasFocused = true;
	const bool focused = glfwGetWindowAttrib(Application::Get()->GetWindow()->GetHandle(), GLFW_FOCUSED) != 0;
	if (focused && !s_WasFocused)
	{
		AssetRegistry::Refresh();
	}
	s_WasFocused = focused;
}

static bool Contains(const std::string& text, const char* filter)
{
	const std::string wanted(filter);
	auto same = [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); };
	return wanted.empty() || std::search(text.begin(), text.end(), wanted.begin(), wanted.end(), same) != text.end();
}

static void ShowFileList(const char* title, const char* explanation, const std::vector<std::string>& files)
{
	if (files.empty() || !ImGui::CollapsingHeader((std::string(title) + " (" + std::to_string(files.size()) + ")").c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	ImGui::TextWrapped("%s", explanation);
	for (const std::string& file : files)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s", file.c_str());
	}
}

void OnImGuiRenderAssetsPanel()
{
	if (!s_ShowAssetsPanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(760.0f, 420.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Assets", &s_ShowAssetsPanel))
	{
		ImGui::End();
		return;
	}
	if (ImGui::Button("Scan Again"))
	{
		AssetRegistry::Refresh();
	}
	const std::vector<AssetRegistry::Record>& records = AssetRegistry::GetRecords();
	ImGui::SameLine();
	ImGui::Text("%d assets, last scan %.1f ms", (int)records.size(), AssetRegistry::GetLastScanMilliseconds());

	// The cook cache: read when the panel opens and after a load, not each frame
	static ModelCook::CacheContents s_Cooked;
	static bool s_CookedKnown = false;
	static uint32_t s_LoadsSeen = 0;
	const ModelCook::Stats& cookStats = ModelCook::GetStats();
	if (!s_CookedKnown || s_LoadsSeen != cookStats.Read + cookStats.Imported)
	{
		s_Cooked = ModelCook::GetCacheContents();
		s_LoadsSeen = cookStats.Read + cookStats.Imported;
		s_CookedKnown = true;
	}
	if (ImGui::Button("Clear Cook Cache"))
	{
		const uint32_t removed = ModelCook::ClearCache() + TextureCook::ClearCache();
		Log::GetLogger()->info("Assets: {0} cooked file(s) removed", removed);
		s_CookedKnown = false;
	}
	ImGui::SameLine();
	ImGui::Text("Cooked models: %u files, %.1f MB in %s", s_Cooked.Files, s_Cooked.Bytes / (1024.0 * 1024.0), s_Cooked.Folder.c_str());
	ImGui::Text("This session: %u read (%.0f ms), %u imported and cooked (%.0f ms)", cookStats.Read, cookStats.ReadMilliseconds, cookStats.Imported, cookStats.ImportMilliseconds);

	// The textures: the folder is counted when a cook finishes or the cache was cleared, not each frame
	const TextureCook::Stats textureStats = TextureCook::GetStats();
	static uint32_t s_TextureFiles = 0;
	static uint64_t s_TextureBytes = 0;
	static uint32_t s_TexturesSeen = UINT32_MAX;
	if (!s_CookedKnown || s_TexturesSeen != textureStats.Cooked)
	{
		TextureCook::CountCachedFiles(s_TextureFiles, s_TextureBytes);
		s_TexturesSeen = textureStats.Cooked;
	}
	ImGui::Text("Cooked textures: %u files, %.1f MB", s_TextureFiles, s_TextureBytes / (1024.0 * 1024.0));
	ImGui::Text("This session: %u read (%.0f ms), %u from their images (%.0f ms), %u cooked (%.0f s), %u failed", textureStats.Read, textureStats.ReadMilliseconds,
		textureStats.FromSource, textureStats.SourceMilliseconds, textureStats.Cooked, textureStats.CookSeconds, textureStats.Failed);
	if (textureStats.Waiting > 0)
	{
		ImGui::Text("Cooking %u: %s", textureStats.Waiting, textureStats.Cooking.c_str());
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Used from their images until cooked; read from the cache the next time they load");
		}
	}

	// What is loaded now, and what uses it
	const std::vector<H2M::ModelH2M::LoadedFile> loadedModels = H2M::ModelH2M::GetLoadedFiles();
	const std::vector<ResourceManager::LoadedTexture> loadedTextures = ResourceManager::GetLoadedTextures();
	uint64_t modelBytes = 0, textureBytes = 0;
	for (const H2M::ModelH2M::LoadedFile& file : loadedModels)
	{
		modelBytes += file.Bytes;
	}
	for (const ResourceManager::LoadedTexture& texture : loadedTextures)
	{
		textureBytes += texture.Bytes;
	}
	const std::string loadedLabel = "Loaded: " + std::to_string(loadedModels.size()) + " model files, " + std::to_string((modelBytes + (1 << 19)) >> 20) + " MB; " +
		std::to_string(loadedTextures.size()) + " textures, " + std::to_string((textureBytes + (1 << 19)) >> 20) + " MB###LoadedAssets";
	if (ImGui::TreeNode(loadedLabel.c_str()))
	{
		if (ImGui::BeginTable("LoadedAssetsTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable, ImVec2(0.0f, 200.0f)))
		{
			ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Used by", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("MB", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableHeadersRow();
			auto row = [](const std::string& path, uint32_t users, uint64_t bytes)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(path.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%u", users);
				ImGui::TableNextColumn();
				ImGui::Text("%.1f", bytes / (1024.0 * 1024.0));
			};
			for (const H2M::ModelH2M::LoadedFile& file : loadedModels)
			{
				row(file.Path, file.Users, file.Bytes);
			}
			for (const ResourceManager::LoadedTexture& texture : loadedTextures)
			{
				row(texture.Path, texture.Users, texture.Bytes);
			}
			ImGui::EndTable();
		}
		ImGui::TreePop();
	}

	ShowFileList("Sidecars without a file", "The file was moved without its .meta, or deleted. What refers to its ID then only has the old path.",
		AssetRegistry::GetOrphanedSidecars());
	ShowFileList("Same ID as another file", "A copied material keeps the ID in its file: left out until it has its own.", AssetRegistry::GetDuplicates());

	static char s_Filter[128] = "";
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##Filter", "Filter by path or type", s_Filter, sizeof(s_Filter));
	if (ImGui::BeginTable("Assets", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 110.0f);
		ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 170.0f);
		ImGui::TableHeadersRow();
		for (const AssetRegistry::Record& record : records)
		{
			const char* typeName = GetAssetTypeName(record.Type);
			if (!Contains(record.File, s_Filter) && !Contains(typeName, s_Filter))
			{
				continue;
			}
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(record.File.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(typeName);
			ImGui::TableNextColumn();
			ImGui::Text("%llu", (unsigned long long)record.ID);
		}
		ImGui::EndTable();
	}
	ImGui::End();
}
