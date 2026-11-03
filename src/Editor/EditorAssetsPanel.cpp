// The Assets panel (docs/engine/Game_Engine_Runtime_Guide.html, 10.1 and the debug view of 10.3), and keeping the registry
// current: files moved in Explorer or by Git are seen when the editor gets the focus back.


#include "Core/Application.h"
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
