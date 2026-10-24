// The Scripts panel (docs/engine/Game_Engine_Runtime_Guide.html, 4.7, its debug view): the project's scripts as the
// editor has them. The loaded assembly (built when, its classes and their fields), the builds and reloads since the
// editor started, the old assemblies still held (leaks), and while the game runs each script instance with its entity and
// the exceptions it threw.


#include "Project/Project.h"
#include "Scripting/ScriptHost.h"
#include "Scripting/ScriptRuntime.h"

#include "imgui.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>
#include <system_error>


#include "EditorState.h"


namespace fs = std::filesystem;

bool s_ShowScriptsPanel = false;

// A file's time as the clock shows it, "14:03:27"
static std::string FormatFileTime(fs::file_time_type time)
{
	const auto system = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
	const std::time_t seconds = std::chrono::system_clock::to_time_t(system);
	std::tm local {};
#if defined(_WIN32)
	localtime_s(&local, &seconds);
#else
	localtime_r(&seconds, &local);
#endif
	char text[32];
	std::strftime(text, sizeof(text), "%H:%M:%S", &local);
	return text;
}

// A path in the project folder relative to it
static std::string ProjectRelative(const fs::path& path)
{
	const Project* project = Project::GetOpen();
	if (project)
	{
		const fs::path relative = path.lexically_relative(fs::u8path(project->GetDirectory()));
		if (!relative.empty() && *relative.begin() != "..")
		{
			return relative.generic_u8string();
		}
	}
	return path.generic_u8string();
}

static void AssemblySection()
{
	if (!ScriptHost::IsAvailable())
	{
		ImGui::TextDisabled("C# scripting isn't available (the log says why)");
		return;
	}
	ImGui::Text(".NET %s", ScriptHost::GetRuntimeVersion().c_str());
	if (!ScriptHost::IsGameAssemblyLoaded())
	{
		const Project* project = Project::GetOpen();
		ImGui::TextDisabled(!project ? "No project: no scripts" : project->ScriptAssembly.empty() ? "The project has no Script Assembly" :
			"The project's scripts aren't loaded (built yet? the log says why)");
		return;
	}
	ImGui::Text("Assembly: %s", ProjectRelative(ScriptHost::GetGameAssemblyPath()).c_str());
	ImGui::Text("Built at %s", FormatFileTime(ScriptHost::GetGameAssemblyWriteTime()).c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The file's time when it was loaded: a newer one is a new build, which is loaded by itself");
	}
	const uint32_t leaked = ScriptHost::GetLeakedAssemblyCount();
	if (leaked > 0)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%u old assembl%s still held (leaks)", leaked, leaked == 1 ? "y" : "ies");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Something still referred to an unloaded assembly's code (a static event subscription, a delegate,\n"
				"a GCHandle): its memory stays in use until the editor ends. The log said so at the reload");
		}
	}

	const std::vector<ScriptClass>& classes = ScriptHost::GetClasses();
	if (ImGui::TreeNodeEx("##Classes", ImGuiTreeNodeFlags_DefaultOpen, "%zu script class(es)", classes.size()))
	{
		for (const ScriptClass& scriptClass : classes)
		{
			if (ImGui::TreeNode(scriptClass.Name.c_str(), "%s (%zu field%s)", scriptClass.Name.c_str(), scriptClass.Fields.size(),
				scriptClass.Fields.size() == 1 ? "" : "s"))
			{
				for (const ScriptFieldInfo& field : scriptClass.Fields)
				{
					ImGui::BulletText("%s %s", field.TypeName.c_str(), field.Name.c_str());
				}
				std::string file;
				int line = 0;
				if (ImGui::SmallButton("Edit Script") && FindScriptClassFile(scriptClass.Name, file, line))
				{
					OpenScriptFile(file, line);
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}
}

static void BuildSection()
{
	if (GetScriptProjectPath().empty())
	{
		ImGui::TextDisabled("The project has no script project to build (Project Settings: Script Project)");
		return;
	}
	ImGui::BeginDisabled(IsScriptBuildRunning());
	if (ImGui::Button("Build Scripts"))
	{
		RequestScriptBuild();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Checkbox("Build on Save", &s_BuildScriptsOnSave))
	{
		SaveEditorSettings();
	}
	const ScriptBuildResult result = GetLastScriptBuild();
	if (IsScriptBuildRunning())
	{
		ImGui::TextDisabled("Building...");
	}
	else if (!result.Finished)
	{
		ImGui::TextDisabled("Not built since the editor started");
	}
	else if (result.Succeeded)
	{
		ImGui::Text("Built in %.1f s%s", result.Seconds, result.Warnings ? (", " + std::to_string(result.Warnings) + " warning(s)").c_str() : "");
	}
	else
	{
		ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.38f, 1.0f), "Failed: %d error(s) (the Console has them)", result.Errors);
		if (!result.FirstMessage.empty() && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", result.FirstMessage.c_str());
		}
	}
}

static void ReloadSection()
{
	const std::vector<ScriptReloadRecord>& history = GetScriptReloadHistory();
	if (history.empty())
	{
		ImGui::TextDisabled("No reloads since the editor started");
		return;
	}
	if (ImGui::BeginTable("##Reloads", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY,
		ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing() * std::min<size_t>(history.size() + 1, 7) + 4.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Time");
		ImGui::TableSetupColumn("Why");
		ImGui::TableSetupColumn("Result");
		ImGui::TableSetupColumn("Time Taken");
		ImGui::TableHeadersRow();
		for (size_t i = history.size(); i-- > 0;)
		{
			const ScriptReloadRecord& record = history[i];
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(record.Time);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(record.Rebuilt ? "A new build" : "Reload");
			ImGui::TableNextColumn();
			if (!record.Loaded)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.38f, 1.0f), "Not loaded (the log says why)");
			}
			else if (record.Leaked)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "%zu classes; the old one is still held", record.ClassCount);
			}
			else
			{
				ImGui::Text("%zu classes", record.ClassCount);
			}
			ImGui::TableNextColumn();
			ImGui::Text("%.0f ms", record.Milliseconds);
		}
		ImGui::EndTable();
	}
}

static void InstancesSection()
{
	const ScriptRuntime* scripts = GetGameScripts();
	if (!scripts)
	{
		ImGui::TextDisabled(IsSimulating() ? "Simulate runs no scripts" : "While the game runs (Play)");
		return;
	}
	ImGui::Text("%zu instance(s), %llu exception(s)", scripts->GetInstanceCount(), (unsigned long long)scripts->GetExceptionCount());
	if (ImGui::BeginTable("##Instances", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Entity");
		ImGui::TableSetupColumn("Class");
		ImGui::TableSetupColumn("Exceptions");
		ImGui::TableHeadersRow();
		for (const ScriptRuntime::Instance& instance : scripts->GetInstances())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const NameComponent* name = s_Scene.TryGet<NameComponent>(instance.Entity);
			ImGui::PushID((const void*)instance.Handle);
			if (ImGui::Selectable(name ? name->Name.c_str() : "(gone)", s_EditorSelection.GetPrimary() == instance.Entity,
				ImGuiSelectableFlags_SpanAllColumns) && s_Scene.Exists(instance.Entity))
			{
				s_EditorSelection.SetPrimary(instance.Entity); // the Properties panel shows its fields as they are now
			}
			ImGui::PopID();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(instance.ClassName.c_str());
			ImGui::TableNextColumn();
			if (instance.ExceptionCount > 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.42f, 0.38f, 1.0f), "%llu, last in %s", (unsigned long long)instance.ExceptionCount,
					instance.LastExceptionMethod.c_str());
			}
			else
			{
				ImGui::TextDisabled("none");
			}
		}
		ImGui::EndTable();
	}
}

void OnImGuiRenderScriptsPanel()
{
	if (!s_ShowScriptsPanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(460.0f, 520.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Scripts", &s_ShowScriptsPanel))
	{
		ImGui::End();
		return;
	}
	ImGui::SeparatorText("Assembly");
	AssemblySection();
	ImGui::SeparatorText("Build");
	BuildSection();
	ImGui::SeparatorText("Reloads");
	ReloadSection();
	ImGui::SeparatorText("Instances");
	InstancesSection();
	ImGui::End();
}
