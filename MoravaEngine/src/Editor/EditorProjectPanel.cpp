// The Project panel: the open project's settings, saved as they are edited


#include "Project/Project.h"
#include "Scripting/ScriptHost.h"

#include "imgui.h"

#include <glm/glm.hpp>

#include <cstring>
#include <string>


#include "EditorState.h"


// A text field for a project's setting: the setting follows each change, true when an edit ended (save it then)
static bool InputSetting(const char* label, std::string& value, const char* hint = nullptr)
{
	char buffer[256];
	strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
	buffer[sizeof(buffer) - 1] = '\0';
	if (hint ? ImGui::InputTextWithHint(label, hint, buffer, sizeof(buffer)) : ImGui::InputText(label, buffer, sizeof(buffer)))
	{
		value = buffer;
	}
	return ImGui::IsItemDeactivatedAfterEdit();
}

// A size of the game's window, in pixels
static bool InputWindowSize(const char* label, uint32_t& value)
{
	int size = (int)value;
	if (ImGui::InputInt(label, &size, 10, 100))
	{
		value = (uint32_t)glm::clamp(size, 320, 7680);
	}
	return ImGui::IsItemDeactivatedAfterEdit();
}

void OnImGuiRenderProjectPanel()
{
	if (!s_ShowProjectPanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(440.0f, 380.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Project", &s_ShowProjectPanel))
	{
		ImGui::End();
		return;
	}

	Project* project = Project::GetOpen();
	if (!project)
	{
		ImGui::TextWrapped("No project is open: scenes, materials and models are the engine's (its MoravaEngine folder).");
		ImGui::Spacing();
		if (ImGui::Button("New Project..."))
		{
			RequestNewProject();
		}
		ImGui::SameLine();
		if (ImGui::Button("Open Project..."))
		{
			RequestOpenProject();
		}
		ImGui::End();
		return;
	}

	bool save = false;
	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
	save |= InputSetting("Name", project->Name);
	ImGui::TextDisabled("%s", project->FilePath.c_str());

	// The scene the game starts with
	ImGui::SeparatorText("Start Scene");
	ImGui::TextUnformatted(project->StartScene.empty() ? "(none: the player has nothing to play)" : project->StartScene.c_str());
	ImGui::BeginDisabled(s_SceneFilePath.empty());
	if (ImGui::Button("Use the Open Scene"))
	{
		project->StartScene = ProjectPaths::ToStored(s_SceneFilePath);
		save = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(s_SceneFilePath.empty() ? "The open scene was never saved: save it first" : "The game starts with the open scene ('%s')",
			s_SceneName.c_str());
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(project->StartScene.empty());
	if (ImGui::Button("Open It"))
	{
		RequestOpenScene(ProjectPaths::Resolve(project->StartScene));
	}
	ImGui::EndDisabled();

	ImGui::SeparatorText("Assets");
	ImGui::Text("Asset folder: %s", project->AssetDirectory.c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s\nThe file dialogs for models, maps and materials start here; new materials are saved in its %s folder.\n"
			"The project's files are found first, then the engine's (its models and environment maps).",
			project->GetAssetFolder().c_str(), Project::MaterialsFolder);
	}

	// The game's C# scripts: loaded again when the path changes (not while the game runs: its scripts hold them)
	ImGui::SeparatorText("Scripts");
	ImGui::BeginDisabled(IsPlaying());
	if (InputSetting("Script Assembly", project->ScriptAssembly, "Scripts/bin/Game.dll"))
	{
		save = true;
		GameSystems::LoadScripts();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("The built DLL of the project's C# scripts, relative to the project folder.\n"
			"Loaded when the project opens and when this path changes; Stop the game first.");
	}
	if (!ScriptHost::IsAvailable())
	{
		ImGui::TextDisabled("C# scripting isn't available (the log says why)");
	}
	else if (ScriptHost::IsGameAssemblyLoaded())
	{
		ImGui::TextDisabled("Loaded: %zu script class(es)", ScriptHost::GetClasses().size());
		if (ImGui::IsItemHovered() && !ScriptHost::GetClasses().empty())
		{
			std::string names;
			for (const ScriptClass& scriptClass : ScriptHost::GetClasses())
			{
				names += (names.empty() ? "" : "\n") + scriptClass.Name;
			}
			ImGui::SetTooltip("%s", names.c_str());
		}
	}
	else if (!project->ScriptAssembly.empty())
	{
		ImGui::TextDisabled("Not loaded (built yet? the log says why)");
	}

	// The player's window
	ImGui::SeparatorText("Game Window");
	save |= InputSetting("Title", project->Window.Title, project->Name.c_str());
	save |= InputWindowSize("Width", project->Window.Width);
	save |= InputWindowSize("Height", project->Window.Height);
	save |= ImGui::Checkbox("Fullscreen", &project->Window.Fullscreen);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The primary monitor, at its resolution (the width and height are the windowed size)");
	}
	save |= ImGui::Checkbox("VSync", &project->Window.VSync);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Frames wait for the display's refresh: no tearing, and no more frames than the display shows");
	}
	ImGui::PopItemWidth();

	ImGui::Spacing();
	ImGui::TextDisabled("The player runs it: MoravaPlayer --project <file>");

	if (save)
	{
		SaveProject();
	}
	ImGui::End();
}
