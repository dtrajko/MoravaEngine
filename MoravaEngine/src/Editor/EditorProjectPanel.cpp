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
			"Loaded when the project opens and when this path changes, and again after each build\n"
			"(a build during Play: at Stop).");
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(IsPlaying() || project->ScriptAssembly.empty() || !ScriptHost::IsAvailable());
	if (ImGui::SmallButton("Reload"))
	{
		RequestScriptReload();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Loads the scripts again now. A new build is loaded by itself; this is for anything else");
	}
	// The scripts' .NET project: built by the editor, opened in the code editor (4.7)
	if (InputSetting("Script Project", project->ScriptProject, "Scripts/Game.csproj"))
	{
		save = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The .csproj of the project's C# scripts, relative to the project folder: the editor builds it\n"
			"(dotnet build) when a script is saved and with Build Scripts, and opens it in the code editor.\n"
			"New projects get one in Scripts/.");
	}
	ImGui::BeginDisabled(project->ScriptProject.empty() || !ScriptHost::IsAvailable());
	ImGui::BeginDisabled(IsScriptBuildRunning());
	if (ImGui::Button(IsScriptBuildRunning() ? "Building..." : "Build Scripts"))
	{
		RequestScriptBuild();
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("dotnet build (Ctrl+B): errors and warnings in the Console, double-click one to open it.\n"
			"A build that succeeds is loaded by itself (during Play: at Stop)");
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Build on Save", &s_BuildScriptsOnSave))
	{
		SaveEditorSettings();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("The scripts are built when one is saved (any .cs in the script project's folder).\n"
			"An editor setting on this computer, not the project's");
	}
	ImGui::SameLine();
	if (ImGui::Button("New Script..."))
	{
		OpenNewScriptDialog();
	}
	if (ImGui::Button("Open in Code Editor"))
	{
		OpenScriptProjectInCodeEditor();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
	const std::string editorPreview = s_CodeEditor == CodeEditorKind::Automatic ?
		std::string("Automatic: ") + GetResolvedCodeEditorName() : GetCodeEditorName(s_CodeEditor);
	if (ImGui::BeginCombo("##CodeEditor", editorPreview.c_str()))
	{
		for (int kind = 0; kind < (int)CodeEditorKind::Count; kind++)
		{
			const bool available = IsCodeEditorAvailable((CodeEditorKind)kind);
			ImGui::BeginDisabled(!available);
			const std::string label = std::string(GetCodeEditorName((CodeEditorKind)kind)) + (available ? "" : " (not found)");
			if (ImGui::Selectable(label.c_str(), s_CodeEditor == (CodeEditorKind)kind))
			{
				s_CodeEditor = (CodeEditorKind)kind;
				SaveEditorSettings();
			}
			ImGui::EndDisabled();
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Where scripts open: the project, an error's line (Console), a class (Properties: Edit Script).\n"
			"Automatic: the first one found of Visual Studio, VS Code and Rider. An editor setting on this computer");
	}
	ImGui::EndDisabled();
	const ScriptBuildResult build = GetLastScriptBuild();
	if (build.Finished && !build.Succeeded && !IsScriptBuildRunning())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.38f, 1.0f), "The last build failed: %d error(s) (Console)", build.Errors);
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
