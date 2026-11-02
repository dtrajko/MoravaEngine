// The Inspector's Script section (docs/engine/Game_Engine_Runtime_Guide.html, 4.5): the entity's C# script class
// and its public fields. Editing, a field set here is saved with the scene and given to the script before its OnCreate;
// the others show the value the class gives them. While the game runs, the fields are the running script's, as they are
// now: a change reaches it at once and lasts until Stop.


#include "Scripting/ScriptHost.h"
#include "Scripting/ScriptRuntime.h"

#include "Project/Project.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>


#include "EditorState.h"


static const ImVec4 s_SetFieldColor(0.45f, 0.75f, 1.0f, 1.0f); // a field the entity sets (not the class's value)
static const ImVec4 s_WarningColor(1.0f, 0.8f, 0.3f, 1.0f);
static const char* s_EntityPayload = "VULKAN_SCENE_ENTITY"; // the Scene Hierarchy's drag & drop of an entity

// "JumpSpeed" as "Jump Speed", "m_speed" as "Speed": a field's name as a label
static std::string NiceName(const std::string& name)
{
	std::string text = name.rfind("m_", 0) == 0 ? name.substr(2) : name;
	text.erase(0, text.find_first_not_of('_'));
	std::string out;
	for (size_t i = 0; i < text.size(); i++)
	{
		const unsigned char c = static_cast<unsigned char>(text[i]);
		const unsigned char previous = i > 0 ? static_cast<unsigned char>(text[i - 1]) : 0;
		const unsigned char next = i + 1 < text.size() ? static_cast<unsigned char>(text[i + 1]) : 0;
		if (i > 0 && std::isupper(c) && (std::islower(previous) || (std::isupper(previous) && std::islower(next))))
		{
			out += ' ';
		}
		out += char(i == 0 ? std::toupper(c) : c);
	}
	return out.empty() ? name : out;
}

static std::string EntityLabel(EnvMapVulkanEntityID entity)
{
	if (entity == NoEntity)
	{
		return "None";
	}
	const NameComponent* name = s_Scene.TryGet<NameComponent>(entity);
	return name ? name->Name : "(not in the scene)";
}

// An entity an Entity field can take: any but a model's parts; a field of a script class (PlayerController Player): only
// entities with that script
static bool CanReference(EnvMapVulkanEntityID entity, const ScriptFieldInfo& field)
{
	if (s_Scene.Has<MeshPartComponent>(entity))
	{
		return false;
	}
	if (field.TypeName == "Entity")
	{
		return true;
	}
	const ScriptComponent* script = s_Scene.TryGet<ScriptComponent>(entity);
	const ScriptClass* scriptClass = script ? ScriptHost::FindClass(script->ClassName) : nullptr;
	return scriptClass && scriptClass->GetShortName() == field.TypeName;
}

static void EntityChoices(const std::vector<EnvMapVulkanEntityID>& entities, const ScriptFieldInfo& field, int depth, ScriptFieldValue& value,
	bool& changed)
{
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (s_Scene.Has<MeshPartComponent>(entity))
		{
			continue;
		}
		if (CanReference(entity, field))
		{
			const std::string label = std::string(size_t(depth) * 2, ' ') + EntityLabel(entity) + "##" + std::to_string(entity);
			if (ImGui::Selectable(label.c_str(), value.Entity == entity))
			{
				value.Entity = entity;
				changed = true;
			}
		}
		EntityChoices(s_Scene.GetChildren(entity), field, depth + 1, value, changed);
	}
}

// A Prefab field's value as a label: the file's name ("Sparkle"), None when empty
static std::string PrefabLabel(const std::string& stored)
{
	return stored.empty() ? "None" : std::filesystem::u8path(stored).stem().u8string();
}

// The prefab files of the project's asset folder (with its subfolders), relative to the project, sorted
static std::vector<std::string> FindProjectPrefabs()
{
	std::vector<std::string> prefabs;
	const Project* project = Project::GetOpen();
	if (!project)
	{
		return prefabs;
	}
	std::error_code error;
	for (std::filesystem::recursive_directory_iterator it(std::filesystem::u8path(project->GetAssetFolder()), error), end; !error && it != end;
		it.increment(error))
	{
		if (it->is_regular_file(error) && IsPrefabFile(it->path().u8string()))
		{
			prefabs.push_back(ProjectPaths::ToStored(it->path().u8string()));
		}
	}
	std::sort(prefabs.begin(), prefabs.end());
	return prefabs;
}

// The widget for a field's value, by its type; true when it changed
static bool FieldWidget(const char* id, const ScriptFieldInfo& field, ScriptFieldValue& value)
{
	bool changed = false;
	switch (field.Type)
	{
	case ScriptFieldType::Bool:
	{
		bool b = value.Integer != 0;
		changed = ImGui::Checkbox(id, &b);
		value.Integer = b ? 1 : 0;
		break;
	}
	case ScriptFieldType::Int:
	{
		int i = int(std::clamp<int64_t>(value.Integer, INT_MIN, INT_MAX));
		changed = ImGui::DragInt(id, &i, 0.2f);
		value.Integer = i;
		break;
	}
	case ScriptFieldType::Float:
	{
		float f = float(value.Number);
		changed = ImGui::DragFloat(id, &f, 0.05f, 0.0f, 0.0f, "%.3f");
		value.Number = f;
		break;
	}
	case ScriptFieldType::Double:
		changed = ImGui::DragScalar(id, ImGuiDataType_Double, &value.Number, 0.05f, nullptr, nullptr, "%.6g");
		break;
	case ScriptFieldType::String:
	{
		char buffer[1024] = {};
		snprintf(buffer, sizeof(buffer), "%s", value.Text.c_str());
		changed = ImGui::InputText(id, buffer, sizeof(buffer));
		value.Text = buffer;
		break;
	}
	case ScriptFieldType::Vector2: changed = ImGui::DragFloat2(id, &value.Vector.x, 0.05f, 0.0f, 0.0f, "%.3f"); break;
	case ScriptFieldType::Vector3: changed = ImGui::DragFloat3(id, &value.Vector.x, 0.05f, 0.0f, 0.0f, "%.3f"); break;
	case ScriptFieldType::Vector4: changed = ImGui::DragFloat4(id, &value.Vector.x, 0.05f, 0.0f, 0.0f, "%.3f"); break;
	case ScriptFieldType::Entity:
		if (ImGui::BeginCombo(id, EntityLabel(value.Entity).c_str()))
		{
			if (ImGui::Selectable("None", value.Entity == NoEntity))
			{
				value.Entity = NoEntity;
				changed = true;
			}
			EntityChoices(s_Scene.GetRoots(), field, 0, value, changed);
			ImGui::EndCombo();
		}
		// An entity dragged from the Scene Hierarchy
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(s_EntityPayload))
			{
				const EnvMapVulkanEntityID dropped = *static_cast<const EnvMapVulkanEntityID*>(payload->Data);
				if (CanReference(dropped, field))
				{
					value.Entity = dropped;
					changed = true;
				}
			}
			ImGui::EndDragDropTarget();
		}
		break;
	case ScriptFieldType::Prefab:
		if (ImGui::BeginCombo(id, PrefabLabel(value.Text).c_str()))
		{
			if (ImGui::Selectable("None", value.Text.empty()))
			{
				value.Text.clear();
				changed = true;
			}
			const std::vector<std::string> prefabs = FindProjectPrefabs();
			for (const std::string& prefab : prefabs)
			{
				if (ImGui::Selectable(prefab.c_str(), prefab == value.Text))
				{
					value.Text = prefab;
					changed = true;
				}
			}
			if (prefabs.empty())
			{
				ImGui::TextDisabled("The project has no prefabs (Scene Hierarchy: Save as Prefab...)");
			}
			ImGui::EndCombo();
		}
		if (ImGui::IsItemHovered() && !value.Text.empty())
		{
			ImGui::SetTooltip("%s", value.Text.c_str());
		}
		// A prefab dragged from the Content Browser
		if (ImGui::BeginDragDropTarget())
		{
			std::string file;
			if (AcceptFileDrop(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IsPrefabFile, "a prefab (.mprefab)", file))
			{
				value.Text = ProjectPaths::ToStored(file);
				changed = true;
			}
			ImGui::EndDragDropTarget();
		}
		break;
	case ScriptFieldType::Enum:
		if (ImGui::BeginCombo(id, value.Text.c_str()))
		{
			for (const std::string& name : field.EnumNames)
			{
				if (ImGui::Selectable(name.c_str(), name == value.Text))
				{
					value.Text = name;
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		break;
	}
	return changed;
}

// A value for a tooltip: "4", "(0, 1, 0)", "Camera"
static std::string ValueText(const ScriptFieldValue& value)
{
	char text[128] = {};
	switch (value.Type)
	{
	case ScriptFieldType::Bool:    return value.Integer != 0 ? "true" : "false";
	case ScriptFieldType::Int:     return std::to_string(value.Integer);
	case ScriptFieldType::Float:
	case ScriptFieldType::Double:  snprintf(text, sizeof(text), "%g", value.Number); return text;
	case ScriptFieldType::Vector2: snprintf(text, sizeof(text), "(%g, %g)", value.Vector.x, value.Vector.y); return text;
	case ScriptFieldType::Vector3: snprintf(text, sizeof(text), "(%g, %g, %g)", value.Vector.x, value.Vector.y, value.Vector.z); return text;
	case ScriptFieldType::Vector4:
		snprintf(text, sizeof(text), "(%g, %g, %g, %g)", value.Vector.x, value.Vector.y, value.Vector.z, value.Vector.w);
		return text;
	case ScriptFieldType::Entity:  return EntityLabel(value.Entity);
	case ScriptFieldType::String:  return "\"" + value.Text + "\"";
	case ScriptFieldType::Enum:    return value.Text;
	case ScriptFieldType::Prefab:  return value.Text.empty() ? "None" : value.Text;
	}
	return "";
}

// The fields of the class: the running script's (instance), or the ones the entity sets with the class's values for the rest
static void FieldControls(ScriptComponent& script, const ScriptClass& scriptClass, void* instance)
{
	if (scriptClass.Fields.empty())
	{
		ImGui::TextDisabled("%s has no public fields", scriptClass.GetShortName().c_str());
		return;
	}
	for (const ScriptFieldInfo& field : scriptClass.Fields)
	{
		ScriptFieldValue value = field.Default;
		const ScriptFieldValue* stored = script.FindField(field.Name);
		bool isSet = false;
		if (instance)
		{
			if (!ScriptHost::GetField(instance, field, value))
			{
				continue;
			}
		}
		else if (stored && stored->ConvertTo(field.Type, value))
		{
			isSet = true;
		}

		ImGui::PushID(field.Name.c_str());
		const bool changed = FieldWidget("##Value", field, value);
		if (changed && instance)
		{
			ScriptHost::SetField(instance, field, value);
		}
		else if (changed)
		{
			script.SetField(field.Name, value);
		}

		// The label after the value, as ImGui's own: highlighted when the entity sets it
		ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
		const std::string label = NiceName(field.Name);
		if (isSet)
		{
			ImGui::TextColored(s_SetFieldColor, "%s", label.c_str());
		}
		else
		{
			ImGui::TextUnformatted(label.c_str());
		}
		if (ImGui::IsItemHovered())
		{
			std::string tip = field.Name + ": " + field.TypeName + "\nThe class's value: " + ValueText(field.Default);
			if (instance)
			{
				tip += "\nThe running script's value: a change lasts until Stop";
			}
			else if (isSet)
			{
				tip += "\nSet by this entity (highlighted). Right-click: back to the class's value";
			}
			ImGui::SetTooltip("%s", tip.c_str());
		}
		if (isSet && ImGui::BeginPopupContextItem("FieldMenu"))
		{
			if (ImGui::MenuItem("Back to the class's value"))
			{
				script.ResetField(field.Name);
			}
			ImGui::EndPopup();
		}
		ImGui::PopID();
	}

	// Values the scene has for fields the class doesn't have (renamed or removed in the code): kept until removed here
	for (size_t i = 0; !instance && i < script.Fields.size(); i++)
	{
		const ScriptComponent::Field& stored = script.Fields[i];
		const ScriptFieldInfo* field = scriptClass.FindField(stored.Name);
		ScriptFieldValue converted;
		if (field && stored.Value.ConvertTo(field->Type, converted))
		{
			continue;
		}
		ImGui::PushID(int(i));
		if (ImGui::SmallButton("Remove"))
		{
			script.ResetField(stored.Name);
			ImGui::PopID();
			break;
		}
		ImGui::SameLine();
		ImGui::TextColored(s_WarningColor, "%s = %s: %s", stored.Name.c_str(), ValueText(stored.Value).c_str(),
			field ? "of another type now" : "not in the class");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The scene sets this field, but %s %s. The value is kept (a field renamed back finds it again)\n"
				"and isn't given to the script. Remove takes it out of the scene.", scriptClass.GetShortName().c_str(),
				field ? ("has it as " + field->TypeName + " now").c_str() : "has no such public field");
		}
		ImGui::PopID();
	}
}

void ScriptPropertiesControls(EnvMapVulkanEntityID entity)
{
	ImGui::Separator();
	ImGui::PushID("Script");

	bool hasScript = s_Scene.Has<ScriptComponent>(entity);
	if (ImGui::Checkbox("Script", &hasScript))
	{
		if (hasScript)
		{
			ScriptComponent& script = s_Scene.Add<ScriptComponent>(entity);
			if (ScriptHost::GetClasses().size() == 1)
			{
				script.ClassName = ScriptHost::GetClasses().front().Name; // the only one there is
			}
		}
		else
		{
			s_Scene.Remove<ScriptComponent>(entity);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A C# script of the project: a class deriving from Morava.Entity. While the game runs, its\n"
			"OnCreate, OnUpdate, OnFixedUpdate and collision methods are called for this entity.");
	}
	ScriptComponent* script = s_Scene.TryGet<ScriptComponent>(entity);

	// Its class's file in the code editor, and a new script for it (4.7)
	if (script && !script->ClassName.empty())
	{
		ImGui::SameLine();
		if (ImGui::SmallButton("Edit Script"))
		{
			std::string file;
			int line = 0;
			if (FindScriptClassFile(script->ClassName, file, line))
			{
				OpenScriptFile(file, line);
			}
			else
			{
				ShowNotification("No file in the scripts' folder declares 'class " + script->ClassName.substr(script->ClassName.rfind('.') + 1) + "'", true);
			}
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Opens the class's file in the code editor (%s), at the class", GetResolvedCodeEditorName());
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(GetScriptProjectPath().empty() || IsPlaying());
	if (ImGui::SmallButton("New Script..."))
	{
		OpenNewScriptDialog(entity);
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(GetScriptProjectPath().empty() ? "The project has no script project (Project Settings: Script Project)" :
			IsPlaying() ? "Not while the game runs: Stop first" :
			"A new script from a template, given to this entity: it opens in the code editor and is built");
	}
	if (!script)
	{
		ImGui::PopID();
		return;
	}

	// The class
	const ScriptClass* scriptClass = ScriptHost::FindClass(script->ClassName);
	const std::string preview = script->ClassName.empty() ? std::string("(choose a class)") :
		scriptClass ? scriptClass->GetShortName() : script->ClassName;
	if (ImGui::BeginCombo("Class", preview.c_str()))
	{
		for (const ScriptClass& candidate : ScriptHost::GetClasses())
		{
			if (ImGui::Selectable(candidate.Name.c_str(), candidate.Name == script->ClassName) && candidate.Name != script->ClassName)
			{
				script->ClassName = candidate.Name;
				script->Fields.clear(); // another class: the old one's values mean nothing to it
			}
		}
		if (ScriptHost::GetClasses().empty())
		{
			ImGui::TextDisabled("No script classes: the project's scripts aren't loaded");
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered() && !script->ClassName.empty())
	{
		ImGui::SetTooltip("%s", script->ClassName.c_str());
	}

	if (!ScriptHost::IsAvailable())
	{
		ImGui::TextColored(s_WarningColor, "C# scripting isn't available (the log says why)");
	}
	else if (!ScriptHost::IsGameAssemblyLoaded())
	{
		ImGui::TextColored(s_WarningColor, "The project's scripts aren't loaded");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("File > Project Settings: Script Assembly is the built DLL of the project's scripts.\n"
				"Without a project there are no scripts.");
		}
	}
	else if (!script->ClassName.empty() && !scriptClass)
	{
		ImGui::TextColored(s_WarningColor, "Not in the game's scripts (renamed, or not built yet)");
	}

	ScriptRuntime* scripts = GetGameScripts();
	void* instance = scripts ? scripts->GetInstance(entity) : nullptr;
	if (IsPlaying())
	{
		ImGui::TextDisabled(instance ? "The running script: changes last until Stop" : "Changes count from the next Play");
	}
	if (scriptClass)
	{
		FieldControls(*script, *scriptClass, instance);
	}
	else
	{
		for (const ScriptComponent::Field& field : script->Fields)
		{
			ImGui::TextDisabled("%s = %s", field.Name.c_str(), ValueText(field.Value).c_str());
		}
	}
	ImGui::PopID();
}
