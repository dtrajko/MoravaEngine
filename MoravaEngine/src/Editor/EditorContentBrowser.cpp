// The Content Browser (docs/engine/Game_Engine_Runtime_Guide.html, 6.1): the open project's folder (or the engine's), as
// icons. Double-click: a folder opens, a scene opens, a prefab is placed where the camera looks, a script opens in the
// code editor, a model is loaded, an .hdr becomes the environment, a material goes to the Material Library, anything
// else opens in the system's program. Drag a file onto the viewport, the Scene Hierarchy, the Inspector or a panel that
// takes it; drag an entity from the Scene Hierarchy here to save it as a prefab in the folder shown. The folder is read
// again every second, so files made elsewhere show up by themselves. Replaces H2M's ContentBrowserPanelH2M, which only
// knew the engine's folder.


#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"
#include "EnvMapVulkan/EnvMapVulkanSceneSerializer.h"

#include "Project/Project.h"

#include "Core/Process.h"

#include "imgui.h"
#include "imgui_internal.h" // BeginDragDropTargetCustom: the whole panel takes an entity

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>


#include "EditorState.h"


namespace fs = std::filesystem;

static const char* s_EntityPayload = "VULKAN_SCENE_ENTITY"; // the Scene Hierarchy's drag & drop of an entity

struct ContentEntry
{
	fs::path Path;
	std::string Name;
	std::string Extension; // lower case, with the dot
	bool Directory = false;
};

static fs::path s_Folder;               // the folder shown (empty: the root)
static bool s_BrowseEngine = false;     // the engine's folder instead of the project's
static float s_IconSize = 56.0f;
static std::vector<ContentEntry> s_Entries;
static fs::path s_ScannedFolder;
static std::chrono::steady_clock::time_point s_LastScan;

static fs::path GetRoot()
{
	const Project* project = Project::GetOpen();
	std::error_code error;
	return project && !s_BrowseEngine ? fs::u8path(project->GetDirectory()) : fs::current_path(error);
}

static std::string Lower(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return text;
}

// The folder's entries: folders first, then files, by name; hidden ones (".vs", ".vscode") left out
static void Scan(const fs::path& folder)
{
	s_Entries.clear();
	std::error_code error;
	for (fs::directory_iterator it(folder, fs::directory_options::skip_permission_denied, error), end; !error && it != end; it.increment(error))
	{
		ContentEntry entry;
		entry.Path = it->path();
		entry.Name = entry.Path.filename().u8string();
		if (entry.Name.empty() || entry.Name.front() == '.')
		{
			continue;
		}
		std::error_code typeError;
		entry.Directory = it->is_directory(typeError);
		entry.Extension = Lower(entry.Path.extension().u8string());
		s_Entries.push_back(entry);
	}
	std::sort(s_Entries.begin(), s_Entries.end(), [](const ContentEntry& a, const ContentEntry& b)
	{
		return a.Directory != b.Directory ? a.Directory : Lower(a.Name) < Lower(b.Name);
	});
	s_ScannedFolder = folder;
	s_LastScan = std::chrono::steady_clock::now();
}

// A file's color by what it is
static ImU32 GetFileColor(const ContentEntry& entry)
{
	const std::string& e = entry.Extension;
	if (e == EnvMapVulkanSceneSerializer::PrefabExtension)                return IM_COL32(115, 190, 255, 255);
	if (e == EnvMapVulkanSceneSerializer::FileExtension)                  return IM_COL32(110, 200, 120, 255);
	if (e == ".cs" || e == ".csproj")                                    return IM_COL32(180, 140, 255, 255);
	if (e == ".wav" || e == ".mp3" || e == ".ogg" || e == ".flac")       return IM_COL32(80, 220, 200, 255);
	if (e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".tga" || e == ".hdr" || e == ".bmp") return IM_COL32(240, 170, 90, 255);
	if (e == ".mmat")                                                    return IM_COL32(255, 120, 170, 255);
	if (e == Project::FileExtension || e == ".minput")                   return IM_COL32(220, 220, 120, 255);
	if (EnvMapVulkanSceneLoader::IsModelFile(entry.Path.u8string()))     return IM_COL32(200, 200, 215, 255);
	return IM_COL32(150, 150, 155, 255);
}

static void DrawIcon(ImDrawList* drawList, const ImVec2& min, float size, const ContentEntry& entry)
{
	const float pad = size * 0.12f;
	const ImVec2 a(min.x + pad, min.y + pad), b(min.x + size - pad, min.y + size - pad);
	if (entry.Directory)
	{
		// A folder: a tab and a body
		const ImU32 color = IM_COL32(230, 190, 90, 255);
		const float tabHeight = (b.y - a.y) * 0.18f;
		drawList->AddRectFilled(ImVec2(a.x, a.y + tabHeight * 0.4f), ImVec2(a.x + (b.x - a.x) * 0.45f, a.y + tabHeight * 1.4f), color, 2.0f);
		drawList->AddRectFilled(ImVec2(a.x, a.y + tabHeight), b, color, 3.0f);
		return;
	}
	// A page with a folded corner, and the extension on it
	const ImU32 color = GetFileColor(entry);
	const float fold = (b.x - a.x) * 0.28f;
	const float inset = (b.x - a.x) * 0.12f;
	const ImVec2 pageA(a.x + inset, a.y), pageB(b.x - inset, b.y);
	drawList->AddRectFilled(pageA, ImVec2(pageB.x - fold, pageB.y), IM_COL32(60, 62, 68, 255), 2.0f);
	drawList->AddRectFilled(ImVec2(pageB.x - fold, pageA.y + fold), pageB, IM_COL32(60, 62, 68, 255), 2.0f);
	drawList->AddTriangleFilled(ImVec2(pageB.x - fold, pageA.y), ImVec2(pageB.x, pageA.y + fold), ImVec2(pageB.x - fold, pageA.y + fold), color);
	drawList->AddRect(pageA, pageB, color, 2.0f, 0, 1.5f);
	std::string label = entry.Extension.size() > 1 ? entry.Extension.substr(1) : std::string("file");
	while (label.size() > 1 && ImGui::CalcTextSize(label.c_str()).x > pageB.x - pageA.x - 4.0f)
	{
		label.pop_back(); // "mproject" on a small page: "mproj"
	}
	const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
	drawList->AddText(ImVec2((pageA.x + pageB.x - textSize.x) * 0.5f, (pageA.y + pageB.y) * 0.5f), color, label.c_str());
}

// What a double-click does with a file
static void OpenEntry(const ContentEntry& entry)
{
	const std::string path = entry.Path.lexically_normal().generic_u8string();
	if (entry.Extension == EnvMapVulkanSceneSerializer::FileExtension)
	{
		RequestOpenScene(path);
	}
	else if (IsPrefabFile(path))
	{
		RequestPrefabOperation(PrefabOperation::Place, NoEntity, path);
	}
	else if (entry.Extension == ".cs")
	{
		OpenScriptFile(path, 1);
	}
	else if (entry.Extension == ".hdr")
	{
		RequestEnvironmentMap(path);
	}
	else if (entry.Extension == ".mmat")
	{
		s_PendingMaterialFiles.push_back(path); // into the Material Library at the start of the next frame
	}
	else if (EnvMapVulkanSceneLoader::IsModelFile(path))
	{
		s_PendingModelFilename = path; // loaded at the start of the next frame, where the camera looks
		s_PendingModelGroundPosition.reset();
	}
	else
	{
		std::string error;
		if (!Process::Open(path, error))
		{
			ShowNotification("The system has no program for '" + entry.Name + "'", true);
		}
	}
}

static const char* GetOpenHint(const ContentEntry& entry)
{
	if (entry.Directory)                                                 return "Double-click: open the folder";
	if (entry.Extension == EnvMapVulkanSceneSerializer::FileExtension)  return "Double-click: open the scene";
	if (IsPrefabFile(entry.Path.u8string()))                            return "Double-click: place it where the camera looks.\nDrag it onto the viewport or the Scene Hierarchy";
	if (entry.Extension == ".cs")                                        return "Double-click: open it in the code editor";
	if (entry.Extension == ".hdr")                                       return "Double-click: the environment map";
	if (entry.Extension == ".mmat")                                      return "Double-click: load it into the Material Library";
	if (EnvMapVulkanSceneLoader::IsModelFile(entry.Path.u8string()))    return "Double-click: load the model.\nDrag it onto the viewport to place it";
	return "Double-click: open it in the system's program. Drag it onto a field that takes it";
}

static void Breadcrumbs(const fs::path& root)
{
	const Project* project = Project::GetOpen();
	if (project)
	{
		if (ImGui::RadioButton("Project", !s_BrowseEngine) && s_BrowseEngine)
		{
			s_BrowseEngine = false;
			s_Folder.clear();
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Engine", s_BrowseEngine) && !s_BrowseEngine)
		{
			s_BrowseEngine = true;
			s_Folder.clear();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The engine's folder: its own models, textures and test scenes");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("|");
		ImGui::SameLine();
	}
	// The root's name, then each folder down to the one shown: click one to go there
	const std::string rootName = project && !s_BrowseEngine ? project->Name : root.filename().u8string();
	if (ImGui::SmallButton(rootName.c_str()))
	{
		s_Folder.clear();
	}
	fs::path walked;
	for (const fs::path& part : s_Folder)
	{
		walked /= part;
		ImGui::SameLine();
		ImGui::TextDisabled("/");
		ImGui::SameLine();
		ImGui::PushID(walked.u8string().c_str());
		if (ImGui::SmallButton(part.u8string().c_str()))
		{
			s_Folder = walked;
		}
		ImGui::PopID();
	}
	ImGui::SameLine(std::max(ImGui::GetCursorPosX(), ImGui::GetContentRegionMax().x - 130.0f));
	ImGui::SetNextItemWidth(120.0f);
	ImGui::SliderFloat("##IconSize", &s_IconSize, 32.0f, 128.0f, "Size %.0f");
}

void OnImGuiRenderContentBrowser()
{
	ImGui::Begin("Content Browser");
	const fs::path root = GetRoot();
	std::error_code error;
	fs::path folder = root / s_Folder;
	if (!fs::is_directory(folder, error))
	{
		s_Folder.clear(); // gone (deleted, another project)
		folder = root;
	}
	Breadcrumbs(root);
	ImGui::Separator();

	if (folder != s_ScannedFolder || std::chrono::steady_clock::now() - s_LastScan > std::chrono::seconds(1))
	{
		Scan(folder);
	}

	ImGui::BeginChild("##Items");
	const float labelHeight = ImGui::GetTextLineHeightWithSpacing();
	const float cellWidth = s_IconSize + 16.0f;
	const int columns = std::max(1, (int)(ImGui::GetContentRegionAvail().x / cellWidth));
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	fs::path enter;
	for (size_t i = 0; i < s_Entries.size(); i++)
	{
		const ContentEntry& entry = s_Entries[i];
		if (i % columns != 0)
		{
			ImGui::SameLine();
		}
		ImGui::PushID((int)i);
		const ImVec2 min = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##entry", ImVec2(cellWidth, s_IconSize + labelHeight));
		const bool hovered = ImGui::IsItemHovered();
		if (hovered)
		{
			drawList->AddRectFilled(min, ImVec2(min.x + cellWidth, min.y + s_IconSize + labelHeight), IM_COL32(255, 255, 255, 25), 4.0f);
		}
		DrawIcon(drawList, ImVec2(min.x + 8.0f, min.y), s_IconSize, entry);
		// The name, cut to the cell (the tooltip has all of it)
		std::string label = entry.Name;
		while (label.size() > 3 && ImGui::CalcTextSize(label.c_str()).x > cellWidth - 4.0f)
		{
			label = label.substr(0, label.size() - 4) + "..";
		}
		const float labelWidth = ImGui::CalcTextSize(label.c_str()).x;
		drawList->AddText(ImVec2(min.x + (cellWidth - labelWidth) * 0.5f, min.y + s_IconSize), IM_COL32(220, 220, 225, 255), label.c_str());

		if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (entry.Directory)
			{
				enter = entry.Path;
			}
			else
			{
				OpenEntry(entry);
			}
		}
		if (hovered && !ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			ImGui::SetTooltip("%s\n%s", entry.Path.lexically_normal().u8string().c_str(), GetOpenHint(entry));
		}
		if (!entry.Directory && ImGui::BeginDragDropSource())
		{
			// The absolute path, as wide characters: every panel that takes a file reads it so
			const std::wstring path = entry.Path.lexically_normal().wstring();
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", path.c_str(), (path.size() + 1) * sizeof(wchar_t));
			ImGui::Text("%s", entry.Name.c_str());
			ImGui::EndDragDropSource();
		}
		ImGui::PopID();
	}
	if (s_Entries.empty())
	{
		ImGui::TextDisabled("(empty)");
	}
	ImGui::EndChild();

	// An entity dropped anywhere on the panel: a prefab of it in the folder shown
	if (ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->InnerRect, ImGui::GetID("##ContentDrop")))
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_EntityPayload))
		{
			const EnvMapVulkanEntityID entity = *(const EnvMapVulkanEntityID*)payload->Data;
			if (s_Scene.Exists(entity))
			{
				RequestPrefabOperation(PrefabOperation::Save, entity, GetNewPrefabFile(folder.generic_u8string(), s_Scene.Get<NameComponent>(entity).Name));
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (!enter.empty())
	{
		s_Folder = fs::relative(enter, root, error);
		if (error)
		{
			s_Folder.clear();
		}
	}
	ImGui::End();
}
