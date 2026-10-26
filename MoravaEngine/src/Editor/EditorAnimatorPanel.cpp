// The Animator panel (View > Animator, docs/engine/Game_Engine_Runtime_Guide.html, 6.4): an animator controller's
// parameters, states and transitions. It shows the selected entity's controller (or one opened from the Content Browser,
// or with Open...), edited in place: the controller is the one the running game plays (AnimatorControllerLibrary), so a
// changed fade or speed shows at once, and an animator keeps its state through an edit by its name. Save writes the
// .manim file; Revert reads it again. While the game runs, the state the selected entity's animator is in is marked, with
// its crossfade.


#include "Animation/AnimationClip.h"
#include "Animation/AnimatorController.h"
#include "Animation/SceneAnimation.h"

#include "EnvMapVulkan/EnvMapVulkanSceneLoader.h"

#include "H2M/Renderer/ModelH2M.h"

#include "Project/Project.h"

#include "Core/FileDialogs.h"
#include "Core/Util.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <system_error>
#include <vector>


#include "EditorState.h"


bool s_ShowAnimatorPanel = false;

static std::string s_File;                              // the controller shown (absolute)
static std::map<std::string, std::string> s_SavedTexts; // each controller's file as last read or written: changes are a difference
static EnvMapVulkanEntityID s_FollowedEntity = NoEntity; // the selected entity the panel last took its controller from

static const ImVec4 s_PlayingColor(0.45f, 0.9f, 0.45f, 1.0f);

static std::string NormalizePath(const std::string& path)
{
	std::error_code error;
	return std::filesystem::weakly_canonical(std::filesystem::u8path(path), error).generic_u8string();
}

static bool IsControllerFile(const std::string& path)
{
	return std::filesystem::u8path(path).extension().u8string() == AnimatorController::FileExtension;
}

void OpenAnimatorController(const std::string& path)
{
	s_File = NormalizePath(path);
	if (std::shared_ptr<AnimatorController> controller = AnimatorControllerLibrary::Get(s_File))
	{
		s_SavedTexts.emplace(s_File, controller->ToText()); // kept when it was opened before: its edits are still unsaved
	}
	s_ShowAnimatorPanel = true;
}

std::string CreateAnimatorController(const H2M::RefH2M<H2M::ModelH2M>& model, const std::string& name)
{
	const Project* project = Project::GetOpen();
	const std::filesystem::path folder = std::filesystem::u8path(project ? project->GetAssetFolder() : std::string("assets")) / "Animators";
	std::error_code error;
	std::filesystem::create_directories(folder, error);
	const std::string base = EnvMapVulkanMaterialLibrary::MakeFileName(name, "Animator");
	std::filesystem::path file = folder / (base + AnimatorController::FileExtension);
	for (int n = 2; std::filesystem::exists(file, error); n++)
	{
		file = folder / (base + "_" + std::to_string(n) + AnimatorController::FileExtension);
	}

	// A state for each of the model's own clips, named after it (or "Clip 2" when the name is taken or empty)
	AnimatorController controller;
	if (model)
	{
		for (size_t c = 0; c < model->GetClips().size(); c++)
		{
			const std::string& clipName = model->GetClips()[c].GetName();
			AnimatorState state;
			state.Name = !clipName.empty() && controller.FindState(clipName) < 0 ? clipName : "Clip " + std::to_string(c + 1);
			state.ClipName = clipName;
			controller.States.push_back(state);
		}
	}
	if (controller.States.empty())
	{
		AnimatorState state;
		state.Name = "Idle";
		controller.States.push_back(state);
	}
	controller.Entry = controller.States.front().Name;

	const std::string path = file.lexically_normal().generic_u8string();
	std::string saveError;
	if (!controller.Save(path, saveError))
	{
		ShowNotification("The controller can't be written: " + saveError, true);
		return std::string();
	}
	return path;
}

// ---- Edits that keep the names consistent: a state or parameter renamed or deleted, everything naming it follows

static void RenameState(AnimatorController& controller, size_t index, const std::string& name)
{
	const std::string old = controller.States[index].Name;
	controller.States[index].Name = name;
	for (AnimatorTransition& transition : controller.Transitions)
	{
		if (transition.From == old) transition.From = name;
		if (transition.To == old) transition.To = name;
	}
	if (controller.Entry == old)
	{
		controller.Entry = name;
	}
}

static void DeleteState(AnimatorController& controller, size_t index)
{
	const std::string name = controller.States[index].Name;
	controller.States.erase(controller.States.begin() + index);
	controller.Transitions.erase(std::remove_if(controller.Transitions.begin(), controller.Transitions.end(), [&](const AnimatorTransition& transition)
	{
		return transition.To == name || (!transition.FromAny && transition.From == name);
	}), controller.Transitions.end());
	if (controller.Entry == name)
	{
		controller.Entry = controller.States.empty() ? std::string() : controller.States.front().Name;
	}
}

static void RenameParameter(AnimatorController& controller, size_t index, const std::string& name)
{
	const std::string old = controller.Parameters[index].Name;
	controller.Parameters[index].Name = name;
	for (AnimatorTransition& transition : controller.Transitions)
	{
		for (AnimatorCondition& condition : transition.Conditions)
		{
			if (condition.Parameter == old) condition.Parameter = name;
		}
	}
	for (AnimatorState& state : controller.States)
	{
		if (state.SpeedParameter == old) state.SpeedParameter = name;
	}
}

static void DeleteParameter(AnimatorController& controller, size_t index)
{
	const std::string name = controller.Parameters[index].Name;
	controller.Parameters.erase(controller.Parameters.begin() + index);
	for (AnimatorTransition& transition : controller.Transitions)
	{
		transition.Conditions.erase(std::remove_if(transition.Conditions.begin(), transition.Conditions.end(),
			[&](const AnimatorCondition& condition) { return condition.Parameter == name; }), transition.Conditions.end());
	}
	for (AnimatorState& state : controller.States)
	{
		if (state.SpeedParameter == name) state.SpeedParameter.clear();
	}
}

// A test that suits the parameter's type: a Float is Greater or Less, a Bool True or False
static void FitConditionTest(const AnimatorController& controller, AnimatorCondition& condition)
{
	const int parameter = controller.FindParameter(condition.Parameter);
	if (parameter < 0)
	{
		return;
	}
	const AnimatorParameterType type = controller.Parameters[parameter].Type;
	const bool boolTest = condition.Test == AnimatorConditionTest::True || condition.Test == AnimatorConditionTest::False;
	if (type == AnimatorParameterType::Float && boolTest)
	{
		condition.Test = AnimatorConditionTest::Greater;
	}
	else if (type == AnimatorParameterType::Bool && !boolTest)
	{
		condition.Test = AnimatorConditionTest::True;
	}
}

static std::string UniqueName(const std::string& base, const std::function<bool(const std::string&)>& taken)
{
	std::string name = base;
	for (int n = 2; taken(name); n++)
	{
		name = base + " " + std::to_string(n);
	}
	return name;
}

// A name field: the new text when it was edited this frame (applied by the caller when valid: not empty, not taken)
static bool NameField(const char* id, const std::string& name, std::string& edited)
{
	char buffer[128] = {};
	snprintf(buffer, sizeof(buffer), "%s", name.c_str());
	if (ImGui::InputText(id, buffer, sizeof(buffer)))
	{
		edited = buffer;
		return true;
	}
	return false;
}

// ---- The sections

static bool ParameterControls(AnimatorController& controller, const SceneAnimation::Info* live)
{
	bool changed = false;
	int deleted = -1;
	for (size_t p = 0; p < controller.Parameters.size(); p++)
	{
		AnimatorParameter& parameter = controller.Parameters[p];
		ImGui::PushID((int)p);
		std::string edited;
		ImGui::SetNextItemWidth(120.0f);
		if (NameField("##Name", parameter.Name, edited) && !edited.empty() && controller.FindParameter(edited) < 0)
		{
			RenameParameter(controller, p, edited);
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		int type = (int)parameter.Type;
		if (ImGui::Combo("##Type", &type, "Float\0Bool\0Trigger\0"))
		{
			parameter.Type = (AnimatorParameterType)type;
			parameter.Default = 0.0f;
			for (AnimatorTransition& transition : controller.Transitions)
			{
				for (AnimatorCondition& condition : transition.Conditions)
				{
					FitConditionTest(controller, condition);
				}
			}
			changed = true;
		}
		ImGui::SameLine();
		if (parameter.Type == AnimatorParameterType::Float)
		{
			ImGui::SetNextItemWidth(70.0f);
			changed |= ImGui::DragFloat("##Default", &parameter.Default, 0.01f);
		}
		else if (parameter.Type == AnimatorParameterType::Bool)
		{
			bool value = parameter.Default != 0.0f;
			if (ImGui::Checkbox("##Default", &value))
			{
				parameter.Default = value ? 1.0f : 0.0f;
				changed = true;
			}
		}
		else
		{
			ImGui::TextDisabled("(set by the game)");
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The value the game starts with");
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
		{
			deleted = (int)p;
		}
		if (live && p < live->Values.size())
		{
			ImGui::SameLine();
			if (parameter.Type == AnimatorParameterType::Float)
			{
				ImGui::TextColored(s_PlayingColor, "%.2f", live->Values[p]);
			}
			else
			{
				ImGui::TextColored(s_PlayingColor, parameter.Type == AnimatorParameterType::Bool ? (live->Values[p] != 0.0f ? "true" : "false")
					: (live->Values[p] != 0.0f ? "set" : "-"));
			}
		}
		ImGui::PopID();
	}
	if (deleted >= 0)
	{
		DeleteParameter(controller, (size_t)deleted);
		changed = true;
	}
	if (ImGui::Button("Add Parameter"))
	{
		AnimatorParameter parameter;
		parameter.Name = UniqueName("Parameter", [&](const std::string& name) { return controller.FindParameter(name) >= 0; });
		controller.Parameters.push_back(parameter);
		changed = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A value the game sets: Animator.SetFloat(\"Speed\", ...), SetBool, SetTrigger (a trigger is set until a\n"
			"transition takes it). Transitions test them; a state's speed can follow a Float");
	}
	return changed;
}

// The clip names of a state's file: the model's own clips (model: the animated entity's, when known), or another file's
static std::vector<std::string> GetClipNames(const AnimatorState& state, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	std::vector<std::string> names;
	if (state.ClipFile.empty())
	{
		if (model)
		{
			for (const AnimationClip& clip : model->GetClips()) names.push_back(clip.GetName());
		}
	}
	else
	{
		for (const AnimationClip& clip : *AnimationClipLibrary::Load(state.ClipFile)) names.push_back(clip.GetName());
	}
	return names;
}

static bool ClipControls(AnimatorState& state, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	bool changed = false;
	const std::string shown = state.ClipFile.empty() ? std::string("(the model's own file)") : ProjectPaths::ToStored(state.ClipFile);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::SetNextItemWidth(-110.0f);
	ImGui::BeginDisabled(true);
	ImGui::InputText("##ClipFile", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			const std::string path = std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data)).lexically_normal().generic_u8string();
			if (EnvMapVulkanSceneLoader::IsModelFile(path))
			{
				state.ClipFile = ProjectPaths::Normalize(path);
				state.ClipName.clear();
				changed = true;
			}
			else
			{
				ShowNotification("Clips come from model files (FBX, glTF, Collada...)", true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("The model file the clip comes from: the animated model's own, or another file of the same rig\n"
			"(Mixamo exports a character's clips as files of their own). Drop a model file here, or choose one with ...");
	}
	ImGui::SameLine();
	if (ImGui::Button("...##ChooseClipFile"))
	{
		const std::string file = FileDialogs::OpenFile({ { "Model with animations", "fbx,gltf,glb,dae,md5mesh,x" } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			state.ClipFile = ProjectPaths::Normalize(std::filesystem::u8path(file).lexically_normal().generic_u8string());
			state.ClipName.clear();
			changed = true;
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(state.ClipFile.empty());
	if (ImGui::Button("Own##OwnClip"))
	{
		state.ClipFile.clear();
		state.ClipName.clear();
		changed = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("A clip of the animated model's own file");
	}

	const std::vector<std::string> names = GetClipNames(state, model);
	ImGui::SetNextItemWidth(-110.0f);
	const std::string current = state.ClipName.empty() ? std::string("(its first clip)") : state.ClipName;
	if (ImGui::BeginCombo("Clip", current.c_str()))
	{
		if (ImGui::Selectable("(its first clip)", state.ClipName.empty()))
		{
			state.ClipName.clear();
			changed = true;
		}
		for (const std::string& name : names)
		{
			if (ImGui::Selectable(name.c_str(), name == state.ClipName))
			{
				state.ClipName = name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	if (names.empty())
	{
		ImGui::TextDisabled(state.ClipFile.empty() && !model ? "Select the animated entity to see its model's clips" : "The file has no clips");
	}
	return changed;
}

static bool StateControls(AnimatorController& controller, const H2M::RefH2M<H2M::ModelH2M>& model, const SceneAnimation::Info* live)
{
	bool changed = false;
	int deleted = -1;
	for (size_t s = 0; s < controller.States.size(); s++)
	{
		AnimatorState& state = controller.States[s];
		ImGui::PushID((int)s);
		const bool playing = live && live->State == state.Name;
		const bool fadingOut = live && live->InTransition && live->PreviousState == state.Name;
		std::string label = state.Name;
		if (controller.Entry == state.Name) label += "  (entry)";
		if (playing) label += live->InTransition ? "  < fading in" : "  < playing";
		if (fadingOut) label += "  < fading out";
		if (playing || fadingOut) ImGui::PushStyleColor(ImGuiCol_Text, s_PlayingColor);
		const bool open = ImGui::TreeNodeEx("##State", ImGuiTreeNodeFlags_SpanAvailWidth, "%s", label.c_str());
		if (playing || fadingOut) ImGui::PopStyleColor();
		if (open)
		{
			std::string edited;
			ImGui::SetNextItemWidth(-110.0f);
			if (NameField("Name", state.Name, edited) && !edited.empty() && controller.FindState(edited) < 0)
			{
				RenameState(controller, s, edited);
				changed = true;
			}
			bool entry = controller.Entry == state.Name;
			if (ImGui::Checkbox("Entry", &entry) && entry)
			{
				controller.Entry = state.Name;
				changed = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The state the animator starts in");
			}
			changed |= ClipControls(state, model);
			ImGui::SetNextItemWidth(80.0f);
			changed |= ImGui::DragFloat("Speed", &state.Speed, 0.01f, -10.0f, 10.0f, "%.3f");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("1: as the clip was made; 2: twice as fast; negative plays it backwards");
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f);
			if (ImGui::BeginCombo("Times", state.SpeedParameter.empty() ? "(nothing)" : state.SpeedParameter.c_str()))
			{
				if (ImGui::Selectable("(nothing)", state.SpeedParameter.empty()))
				{
					state.SpeedParameter.clear();
					changed = true;
				}
				for (const AnimatorParameter& parameter : controller.Parameters)
				{
					if (parameter.Type == AnimatorParameterType::Float && ImGui::Selectable(parameter.Name.c_str(), parameter.Name == state.SpeedParameter))
					{
						state.SpeedParameter = parameter.Name;
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The speed is multiplied by this Float parameter: a walk set to the character's speed keeps its feet\n"
					"from sliding (Speed 1 / the clip's own meters per second, times Speed in meters per second)");
			}
			changed |= ImGui::Checkbox("Loop", &state.Loop);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Off: the clip stops on its last frame (a transition with an exit time leaves it then)");
			}
			ImGui::SameLine();
			changed |= ImGui::Checkbox("In Place", &state.InPlace);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("The clip's root (the hips) keeps its place across the ground: for a walk or run made with root\n"
					"motion, which the character controller moves instead (moving the entity by the clip comes in Phase 10)");
			}
			if (playing)
			{
				ImGui::TextColored(s_PlayingColor, "%.2f s played (%.2f of the clip)", live->StateTime, live->NormalizedTime);
			}
			if (ImGui::SmallButton("Delete State"))
			{
				deleted = (int)s;
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (deleted >= 0)
	{
		DeleteState(controller, (size_t)deleted);
		changed = true;
	}
	if (ImGui::Button("Add State"))
	{
		AnimatorState state;
		state.Name = UniqueName("State", [&](const std::string& name) { return controller.FindState(name) >= 0; });
		controller.States.push_back(state);
		if (controller.Entry.empty())
		{
			controller.Entry = state.Name;
		}
		changed = true;
	}
	return changed;
}

static bool ConditionControls(AnimatorController& controller, AnimatorTransition& transition)
{
	bool changed = false;
	int deleted = -1;
	for (size_t c = 0; c < transition.Conditions.size(); c++)
	{
		AnimatorCondition& condition = transition.Conditions[c];
		ImGui::PushID((int)c);
		ImGui::SetNextItemWidth(110.0f);
		if (ImGui::BeginCombo("##Parameter", condition.Parameter.c_str()))
		{
			for (const AnimatorParameter& parameter : controller.Parameters)
			{
				if (ImGui::Selectable(parameter.Name.c_str(), parameter.Name == condition.Parameter))
				{
					condition.Parameter = parameter.Name;
					FitConditionTest(controller, condition);
					changed = true;
				}
			}
			ImGui::EndCombo();
		}
		const int parameter = controller.FindParameter(condition.Parameter);
		const AnimatorParameterType type = parameter >= 0 ? controller.Parameters[parameter].Type : AnimatorParameterType::Float;
		ImGui::SameLine();
		if (type == AnimatorParameterType::Float)
		{
			int test = condition.Test == AnimatorConditionTest::Less ? 1 : 0;
			ImGui::SetNextItemWidth(70.0f);
			if (ImGui::Combo("##Test", &test, "Greater\0Less\0"))
			{
				condition.Test = test == 1 ? AnimatorConditionTest::Less : AnimatorConditionTest::Greater;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(60.0f);
			changed |= ImGui::DragFloat("##Value", &condition.Value, 0.01f);
		}
		else if (type == AnimatorParameterType::Bool)
		{
			int test = condition.Test == AnimatorConditionTest::False ? 1 : 0;
			ImGui::SetNextItemWidth(70.0f);
			if (ImGui::Combo("##Test", &test, "True\0False\0"))
			{
				condition.Test = test == 1 ? AnimatorConditionTest::False : AnimatorConditionTest::True;
				changed = true;
			}
		}
		else
		{
			ImGui::TextDisabled("is set (and is used up)");
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
		{
			deleted = (int)c;
		}
		ImGui::PopID();
	}
	if (deleted >= 0)
	{
		transition.Conditions.erase(transition.Conditions.begin() + deleted);
		changed = true;
	}
	ImGui::BeginDisabled(controller.Parameters.empty());
	if (ImGui::SmallButton("Add Condition"))
	{
		AnimatorCondition condition;
		condition.Parameter = controller.Parameters.front().Name;
		FitConditionTest(controller, condition);
		transition.Conditions.push_back(condition);
		changed = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(controller.Parameters.empty() ? "Add a parameter first" : "Every condition has to hold");
	}
	return changed;
}

static bool StateCombo(const char* label, const AnimatorController& controller, std::string& state, bool* any)
{
	bool changed = false;
	const std::string shown = any && *any ? std::string("Any State") : state;
	ImGui::SetNextItemWidth(130.0f);
	if (ImGui::BeginCombo(label, shown.c_str()))
	{
		if (any && ImGui::Selectable("Any State", *any))
		{
			*any = true;
			changed = true;
		}
		for (const AnimatorState& candidate : controller.States)
		{
			if (ImGui::Selectable(candidate.Name.c_str(), (!any || !*any) && candidate.Name == state))
			{
				state = candidate.Name;
				if (any) *any = false;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

static bool TransitionControls(AnimatorController& controller, const SceneAnimation::Info* live)
{
	bool changed = false;
	int deleted = -1;
	for (size_t t = 0; t < controller.Transitions.size(); t++)
	{
		AnimatorTransition& transition = controller.Transitions[t];
		ImGui::PushID((int)t);
		const std::string from = transition.FromAny ? std::string("Any State") : transition.From;
		const bool running = live && live->InTransition && live->State == transition.To &&
			(transition.FromAny || live->PreviousState == transition.From);
		if (running) ImGui::PushStyleColor(ImGuiCol_Text, s_PlayingColor);
		const bool open = ImGui::TreeNodeEx("##Transition", ImGuiTreeNodeFlags_SpanAvailWidth, "%s -> %s%s", from.c_str(), transition.To.c_str(),
			running ? "  < crossfading" : "");
		if (running) ImGui::PopStyleColor();
		if (open)
		{
			changed |= StateCombo("From", controller, transition.From, &transition.FromAny);
			ImGui::SameLine();
			changed |= StateCombo("To", controller, transition.To, nullptr);
			ImGui::SetNextItemWidth(80.0f);
			if (ImGui::DragFloat("Fade", &transition.Fade, 0.01f, 0.0f, 5.0f, "%.2f s"))
			{
				transition.Fade = std::max(transition.Fade, 0.0f);
				changed = true;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Seconds of crossfade: the state it comes from goes on playing, blended away (0: a cut)");
			}
			changed |= ImGui::Checkbox("Exit Time", &transition.HasExitTime);
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Only once the state has played that far: 1 is its clip's end (a looped state below 1: that far\n"
					"in each loop). Without conditions, it is taken then: Use, then back to Idle");
			}
			if (transition.HasExitTime)
			{
				ImGui::SameLine();
				ImGui::SetNextItemWidth(70.0f);
				if (ImGui::DragFloat("##ExitTime", &transition.ExitTime, 0.01f, 0.0f, 10.0f, "%.2f"))
				{
					transition.ExitTime = std::max(transition.ExitTime, 0.0f);
					changed = true;
				}
			}
			ImGui::TextDisabled("When:");
			changed |= ConditionControls(controller, transition);
			if (transition.Conditions.empty() && !transition.HasExitTime)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "No condition and no exit time: taken at once");
			}
			if (ImGui::SmallButton("Delete Transition"))
			{
				deleted = (int)t;
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
	if (deleted >= 0)
	{
		controller.Transitions.erase(controller.Transitions.begin() + deleted);
		changed = true;
	}
	ImGui::BeginDisabled(controller.States.empty());
	if (ImGui::Button("Add Transition"))
	{
		AnimatorTransition transition;
		transition.From = controller.States.front().Name;
		transition.To = controller.States.size() > 1 ? controller.States[1].Name : controller.States.front().Name;
		controller.Transitions.push_back(transition);
		changed = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("Each frame the first transition that may be taken is: those from Any State first, then the current\n"
			"state's, in this order");
	}
	return changed;
}

// The selected entity with an animator: the model's entity when a part of it is selected
static EnvMapVulkanEntityID GetSelectedAnimatorEntity()
{
	const EnvMapVulkanEntityID model = GetSelectedModel();
	if (model != NoEntity && s_Scene.Has<AnimatorComponent>(model))
	{
		return model;
	}
	const EnvMapVulkanEntityID primary = s_EditorSelection.GetPrimary();
	return primary != NoEntity && s_Scene.Exists(primary) && s_Scene.Has<AnimatorComponent>(primary) ? primary : NoEntity;
}

void OnImGuiRenderAnimatorPanel()
{
	if (!s_ShowAnimatorPanel)
	{
		return;
	}
	if (!ImGui::Begin("Animator", &s_ShowAnimatorPanel))
	{
		ImGui::End();
		return;
	}

	// The selected entity's controller, when another entity is selected
	const EnvMapVulkanEntityID entity = GetSelectedAnimatorEntity();
	if (entity != s_FollowedEntity)
	{
		s_FollowedEntity = entity;
		if (entity != NoEntity && !s_Scene.Get<AnimatorComponent>(entity).Controller.empty())
		{
			OpenAnimatorController(s_Scene.Get<AnimatorComponent>(entity).Controller);
		}
	}

	if (ImGui::Button("Open..."))
	{
		const std::string file = FileDialogs::OpenFile({ { "Animator controller", "manim" } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			OpenAnimatorController(file);
		}
	}
	std::shared_ptr<AnimatorController> controller = s_File.empty() ? nullptr : AnimatorControllerLibrary::Get(s_File);
	if (!controller)
	{
		ImGui::SameLine();
		ImGui::TextDisabled(s_File.empty() ? "Select an entity with an Animator (Inspector), or open a .manim file" : "The controller can't be read (the log says why)");
		ImGui::End();
		return;
	}

	// Its file, and whether it has unsaved changes
	const bool unsaved = controller->ToText() != s_SavedTexts[s_File];
	ImGui::SameLine();
	ImGui::BeginDisabled(!unsaved);
	if (ImGui::Button("Save"))
	{
		std::string error;
		if (controller->Save(s_File, error))
		{
			s_SavedTexts[s_File] = controller->ToText();
		}
		else
		{
			ShowNotification(error, true);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Revert"))
	{
		std::string error;
		if (AnimatorControllerLibrary::Reload(s_File, error))
		{
			s_SavedTexts[s_File] = controller->ToText();
		}
		else
		{
			ShowNotification("The controller can't be read again: " + error, true);
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextUnformatted(std::filesystem::u8path(s_File).filename().u8string().c_str());
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", s_File.c_str());
	}
	if (unsaved)
	{
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "(unsaved changes)");
	}

	// The animated entity this controller belongs to (its model's clips, its animator in the running game)
	const bool followed = s_FollowedEntity != NoEntity && NormalizePath(s_Scene.Get<AnimatorComponent>(s_FollowedEntity).Controller) == s_File;
	const H2M::RefH2M<H2M::ModelH2M> model = followed && s_Scene.Has<ModelComponent>(s_FollowedEntity) ?
		s_Scene.Get<ModelComponent>(s_FollowedEntity).Model : H2M::RefH2M<H2M::ModelH2M>();
	SceneAnimation::Info info;
	SceneAnimation* game = GetGameAnimation();
	const SceneAnimation::Info* live = followed && game && game->GetInfo(s_FollowedEntity, info) ? &info : nullptr;
	if (followed)
	{
		ImGui::TextDisabled("The animator of '%s'", s_Scene.Get<NameComponent>(s_FollowedEntity).Name.c_str());
	}
	if (live)
	{
		if (!live->Problem.empty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "It doesn't animate: %s", live->Problem.c_str());
		}
		else if (live->InTransition)
		{
			const std::string from = live->PreviousState.empty() ? std::string("the pose before") : live->PreviousState;
			ImGui::TextColored(s_PlayingColor, "%s -> %s", from.c_str(), live->State.c_str());
			ImGui::SameLine();
			ImGui::ProgressBar(live->FadeProgress, ImVec2(-1.0f, 0.0f));
		}
		else
		{
			ImGui::TextColored(s_PlayingColor, "In %s", live->State.c_str());
		}
	}

	bool changed = false;
	ImGui::Separator();
	if (ImGui::CollapsingHeader("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed |= ParameterControls(*controller, live);
	}
	if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed |= StateControls(*controller, model, live);
	}
	if (ImGui::CollapsingHeader("Transitions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed |= TransitionControls(*controller, live);
	}
	if (changed)
	{
		controller->MarkChanged(); // the running game takes it at its next frame
	}
	ImGui::End();
}
