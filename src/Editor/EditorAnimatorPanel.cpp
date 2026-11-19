// The Animator panel (View > Animator, docs/engine/Game_Engine_Runtime_Guide.html, 6.4): an animator controller's
// parameters, and the states and transitions of its base or of one of its layers (the Layer combo; a layer's mask and
// weight with it). It shows the selected entity's controller (or one opened from the Content Browser,
// or with Open...), edited in place: the controller is the one the running game plays (AnimatorControllerLibrary), so a
// changed fade or speed shows at once, and an animator keeps its state through an edit by its name. Save writes the
// .manim file; Revert reads it again. While the game runs, the state the selected entity's animator is in is marked, with
// its crossfade.


#include "Animation/AnimationClip.h"
#include "Core/EnginePaths.h"
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
static int s_Layer = -1;                                 // the layer shown (-1: the base)

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
	if (NormalizePath(path) != s_File)
	{
		s_Layer = -1; // another controller: its base first
	}
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
	const std::filesystem::path folder = std::filesystem::u8path(project ? project->GetAssetFolder() : std::string(ASSETS_DIR)) / "Animators";
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

static void RenameState(AnimatorStateMachine& controller, size_t index, const std::string& name)
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

static void DeleteState(AnimatorStateMachine& controller, size_t index)
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

// The base and every layer's state machine
static std::vector<AnimatorStateMachine*> GetMachines(AnimatorController& controller)
{
	std::vector<AnimatorStateMachine*> machines = { &controller };
	for (AnimatorLayer& layer : controller.Layers)
	{
		machines.push_back(&layer);
	}
	return machines;
}

static void RenameParameter(AnimatorController& controller, size_t index, const std::string& name)
{
	const std::string old = controller.Parameters[index].Name;
	controller.Parameters[index].Name = name;
	for (AnimatorStateMachine* machine : GetMachines(controller))
	{
		for (AnimatorTransition& transition : machine->Transitions)
		{
			for (AnimatorCondition& condition : transition.Conditions)
			{
				if (condition.Parameter == old) condition.Parameter = name;
			}
		}
		for (AnimatorState& state : machine->States)
		{
			if (state.SpeedParameter == old) state.SpeedParameter = name;
			if (state.BlendParameter == old) state.BlendParameter = name;
		}
	}
	for (AnimatorLayer& layer : controller.Layers)
	{
		if (layer.WeightParameter == old) layer.WeightParameter = name;
	}
}

static void DeleteParameter(AnimatorController& controller, size_t index)
{
	const std::string name = controller.Parameters[index].Name;
	controller.Parameters.erase(controller.Parameters.begin() + index);
	for (AnimatorStateMachine* machine : GetMachines(controller))
	{
		for (AnimatorTransition& transition : machine->Transitions)
		{
			transition.Conditions.erase(std::remove_if(transition.Conditions.begin(), transition.Conditions.end(),
				[&](const AnimatorCondition& condition) { return condition.Parameter == name; }), transition.Conditions.end());
		}
		for (AnimatorState& state : machine->States)
		{
			if (state.SpeedParameter == name) state.SpeedParameter.clear();
			if (state.BlendParameter == name) state.BlendParameter.clear();
		}
	}
	for (AnimatorLayer& layer : controller.Layers)
	{
		if (layer.WeightParameter == name) layer.WeightParameter.clear();
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
			for (AnimatorStateMachine* machine : GetMachines(controller))
			{
				for (AnimatorTransition& transition : machine->Transitions)
				{
					for (AnimatorCondition& condition : transition.Conditions)
					{
						FitConditionTest(controller, condition);
					}
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
static std::vector<std::string> GetClipNames(const std::string& clipFile, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	std::vector<std::string> names;
	if (clipFile.empty())
	{
		if (model)
		{
			for (const AnimationClip& clip : model->GetClips()) names.push_back(clip.GetName());
		}
	}
	else
	{
		for (const AnimationClip& clip : *AnimationClipLibrary::Load(clipFile)) names.push_back(clip.GetName());
	}
	return names;
}

// A clip: its model file and its name there (a state's, or one of a blend state's)
static bool ClipControls(std::string& clipFile, std::string& clipName, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	bool changed = false;
	const std::string shown = clipFile.empty() ? std::string("(the model's own file)") : ProjectPaths::ToStored(clipFile);
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
				clipFile = ProjectPaths::Normalize(path);
				clipName.clear();
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
			clipFile = ProjectPaths::Normalize(std::filesystem::u8path(file).lexically_normal().generic_u8string());
			clipName.clear();
			changed = true;
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(clipFile.empty());
	if (ImGui::Button("Own##OwnClip"))
	{
		clipFile.clear();
		clipName.clear();
		changed = true;
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("A clip of the animated model's own file");
	}

	const std::vector<std::string> names = GetClipNames(clipFile, model);
	ImGui::SetNextItemWidth(-110.0f);
	const std::string current = clipName.empty() ? std::string("(its first clip)") : clipName;
	if (ImGui::BeginCombo("Clip", current.c_str()))
	{
		if (ImGui::Selectable("(its first clip)", clipName.empty()))
		{
			clipName.clear();
			changed = true;
		}
		for (const std::string& name : names)
		{
			if (ImGui::Selectable(name.c_str(), name == clipName))
			{
				clipName = name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	if (names.empty())
	{
		ImGui::TextDisabled(clipFile.empty() && !model ? "Select the animated entity to see its model's clips" : "The file has no clips");
	}
	return changed;
}

// A state's events: a name and a time in its clip each (0 its start, 1 its end), put in time order once a time is edited
static bool EventControls(AnimatorState& state, const SceneAnimation::LayerInfo* live)
{
	bool changed = false;
	bool sort = false;
	int deleted = -1;
	ImGui::TextUnformatted("Events");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Moments of the clip the state's scripts hear of (Entity.OnAnimationEvent): a foot down (Footstep),\n"
			"a take-off. In a looped state they come in each loop; during a crossfade only the state that weighs more fires");
	}
	for (size_t e = 0; e < state.Events.size(); e++)
	{
		AnimatorEvent& event = state.Events[e];
		ImGui::PushID((int)e);
		std::string edited;
		ImGui::SetNextItemWidth(120.0f);
		if (NameField("##Name", event.Name, edited) && !edited.empty())
		{
			event.Name = edited;
			changed = true;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);
		changed |= ImGui::DragFloat("##Time", &event.Time, 0.002f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		sort |= ImGui::IsItemDeactivatedAfterEdit();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Where in the clip: 0 its start, 1 its end");
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
		{
			deleted = (int)e;
		}
		ImGui::PopID();
	}
	if (deleted >= 0)
	{
		state.Events.erase(state.Events.begin() + deleted);
		changed = true;
	}
	const bool playing = live && live->State == state.Name;
	const float now = playing ? live->NormalizedTime - std::floor(live->NormalizedTime) : 0.0f;
	if (ImGui::SmallButton(playing ? "Add Event Here" : "Add Event"))
	{
		state.Events.push_back({ state.Events.empty() ? std::string("Footstep") : state.Events.back().Name, playing ? now : 0.0f });
		changed = sort = true;
	}
	if (ImGui::IsItemHovered())
	{
		if (playing)
		{
			ImGui::SetTooltip("An event where the clip is now (%.3f): Pause the game to pick the moment", now);
		}
		else
		{
			ImGui::SetTooltip("An event at the clip's start");
		}
	}
	if (sort)
	{
		AnimatorController::SortEvents(state);
	}
	return changed;
}

// A Float parameter of the controller, or nothing (none: "(nothing)" shown)
static bool FloatParameterCombo(const char* label, const AnimatorController& controller, std::string& parameter, const char* none)
{
	bool changed = false;
	if (ImGui::BeginCombo(label, parameter.empty() ? none : parameter.c_str()))
	{
		if (ImGui::Selectable(none, parameter.empty()))
		{
			parameter.clear();
			changed = true;
		}
		for (const AnimatorParameter& candidate : controller.Parameters)
		{
			if (candidate.Type == AnimatorParameterType::Float && ImGui::Selectable(candidate.Name.c_str(), candidate.Name == parameter))
			{
				parameter = candidate.Name;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}

// A state's clip, or a blend state's clips with their thresholds and the parameter blending them
static bool StateClipControls(const AnimatorController& controller, AnimatorState& state, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	bool changed = false;
	bool blend = !state.Blend.empty();
	if (ImGui::Checkbox("Blend", &blend))
	{
		if (blend)
		{
			// Its clip becomes the blend's first, blended by the first Float parameter
			state.Blend.push_back({ state.ClipFile, state.ClipName, 0.0f });
			for (const AnimatorParameter& parameter : controller.Parameters)
			{
				if (parameter.Type == AnimatorParameterType::Float)
				{
					state.BlendParameter = parameter.Name;
					break;
				}
			}
		}
		else
		{
			// The blend's first clip becomes its clip
			state.ClipFile = state.Blend.front().ClipFile;
			state.ClipName = state.Blend.front().ClipName;
			state.Blend.clear();
			state.BlendParameter.clear();
		}
		changed = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Clips blended by a Float parameter: between the two whose thresholds it lies between (an aim\n"
			"up, ahead and down by the look's pitch; a walk by its direction). They play in step, each at the same\n"
			"fraction of its length");
	}
	if (!blend)
	{
		return ClipControls(state.ClipFile, state.ClipName, model) || changed;
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(130.0f);
	changed |= FloatParameterCombo("By", controller, state.BlendParameter, "(no parameter)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The Float parameter whose value picks the clips (the scripts set it: Animator.SetFloat)");
	}
	if (state.BlendParameter.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "No parameter: it shows its first clip");
	}
	bool sort = false;
	int deleted = -1;
	for (size_t c = 0; c < state.Blend.size(); c++)
	{
		AnimatorBlendClip& clip = state.Blend[c];
		ImGui::PushID((int)c);
		ImGui::SetNextItemWidth(80.0f);
		changed |= ImGui::DragFloat("##Threshold", &clip.Threshold, 0.1f, -10000.0f, 10000.0f, "%.2f");
		sort |= ImGui::IsItemDeactivatedAfterEdit();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The parameter's value where this clip alone shows");
		}
		ImGui::SameLine();
		if (ImGui::SmallButton("x"))
		{
			deleted = (int)c;
		}
		ImGui::Indent();
		changed |= ClipControls(clip.ClipFile, clip.ClipName, model);
		ImGui::Unindent();
		ImGui::PopID();
	}
	if (deleted >= 0 && state.Blend.size() > 1)
	{
		state.Blend.erase(state.Blend.begin() + deleted);
		changed = true;
	}
	if (ImGui::SmallButton("Add Clip"))
	{
		AnimatorBlendClip clip = state.Blend.back();
		clip.Threshold += 1.0f;
		state.Blend.push_back(clip);
		changed = true;
	}
	if (sort)
	{
		AnimatorController::SortBlend(state);
	}
	return changed;
}

static bool StateControls(AnimatorController& parameters, AnimatorStateMachine& controller, const H2M::RefH2M<H2M::ModelH2M>& model,
	const SceneAnimation::LayerInfo* live)
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
			changed |= StateClipControls(parameters, state, model);
			ImGui::SetNextItemWidth(80.0f);
			changed |= ImGui::DragFloat("Speed", &state.Speed, 0.01f, -10.0f, 10.0f, "%.3f");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("1: as the clip was made; 2: twice as fast; negative plays it backwards");
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f);
			changed |= FloatParameterCombo("Times", parameters, state.SpeedParameter, "(nothing)");
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
					"motion, which the character controller moves instead (moving the entity by the clip comes in Phase 12)");
			}
			if (playing)
			{
				ImGui::TextColored(s_PlayingColor, "%.2f s played (%.2f of the clip)", live->StateTime, live->NormalizedTime);
			}
			changed |= EventControls(state, live);
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

static bool StateCombo(const char* label, const AnimatorStateMachine& controller, std::string& state, bool* any)
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

static bool TransitionControls(AnimatorController& parameters, AnimatorStateMachine& controller, const SceneAnimation::LayerInfo* live)
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
			changed |= ConditionControls(parameters, transition);
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

// The layer shown (the base or a layer, added and deleted here) and a layer's mask and weight
static bool LayerControls(AnimatorController& controller, const H2M::RefH2M<H2M::ModelH2M>& model, const SceneAnimation::Info* live)
{
	bool changed = false;
	auto layerName = [&](int layer) { return layer < 0 ? std::string("Base") : controller.Layers[layer].Name; };
	ImGui::SetNextItemWidth(160.0f);
	if (ImGui::BeginCombo("Layer", layerName(s_Layer).c_str()))
	{
		for (int layer = -1; layer < (int)controller.Layers.size(); layer++)
		{
			std::string label = layerName(layer);
			if (live && layer >= 0 && layer < (int)live->Layers.size())
			{
				label += "  < " + live->Layers[layer].State;
			}
			if (ImGui::Selectable(label.c_str(), layer == s_Layer))
			{
				s_Layer = layer;
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The base poses every node; each layer over it has its own states and transitions, and takes over\n"
			"the nodes of its mask by its weight (an upper body that aims while the legs walk). The parameters are\n"
			"all layers'");
	}
	ImGui::SameLine();
	if (ImGui::Button("Add Layer"))
	{
		AnimatorLayer layer;
		layer.Name = UniqueName("Layer", [&](const std::string& name) { return controller.FindLayer(name) >= 0; });
		controller.Layers.push_back(layer);
		s_Layer = (int)controller.Layers.size() - 1;
		changed = true;
	}
	if (s_Layer < 0 || s_Layer >= (int)controller.Layers.size())
	{
		return changed;
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete Layer"))
	{
		controller.Layers.erase(controller.Layers.begin() + s_Layer);
		s_Layer = -1;
		return true;
	}

	AnimatorLayer& layer = controller.Layers[s_Layer];
	ImGui::PushID("Layer");
	std::string edited;
	ImGui::SetNextItemWidth(-110.0f);
	if (NameField("Name", layer.Name, edited) && !edited.empty() && controller.FindLayer(edited) < 0)
	{
		layer.Name = edited;
		changed = true;
	}
	// The mask: node names, each with what is under it (the model's nodes, when the animated entity is selected)
	std::string mask;
	for (const std::string& node : layer.Mask)
	{
		mask += (mask.empty() ? "" : ", ") + node;
	}
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", mask.c_str());
	ImGui::SetNextItemWidth(-110.0f);
	if (ImGui::InputTextWithHint("Mask", "(all nodes)", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue) ||
		ImGui::IsItemDeactivatedAfterEdit())
	{
		std::vector<std::string> nodes;
		std::string current;
		for (const char* c = buffer;; c++)
		{
			if (*c == ',' || *c == '\0')
			{
				const size_t first = current.find_first_not_of(" \t"), last = current.find_last_not_of(" \t");
				if (first != std::string::npos)
				{
					nodes.push_back(current.substr(first, last - first + 1));
				}
				current.clear();
				if (*c == '\0') break;
			}
			else
			{
				current += *c;
			}
		}
		if (nodes != layer.Mask)
		{
			layer.Mask = nodes;
			changed = true;
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The nodes it moves, separated by commas: each with every node under it (spine_01: the upper body).\n"
			"Empty: all of them");
	}
	const AnimationSkeleton* skeleton = model ? model->GetSkeleton() : nullptr;
	for (const std::string& node : layer.Mask)
	{
		if (skeleton && skeleton->FindNode(node) < 0)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "The model has no node '%s'", node.c_str());
		}
	}
	ImGui::SetNextItemWidth(120.0f);
	changed |= ImGui::SliderFloat("Weight", &layer.Weight, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How much it takes over its nodes: 1 all of them, 0 none");
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(130.0f);
	changed |= FloatParameterCombo("Times##Weight", controller, layer.WeightParameter, "(nothing)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The weight is multiplied by this Float parameter (0 to 1): a layer the scripts fade in and out");
	}
	if (live && s_Layer < (int)live->Layers.size())
	{
		ImGui::TextColored(s_PlayingColor, "Weight now %.2f", live->Layers[s_Layer].Weight);
	}
	ImGui::PopID();
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
	changed |= LayerControls(*controller, model, live);
	if (s_Layer >= (int)controller->Layers.size())
	{
		s_Layer = -1;
	}
	AnimatorStateMachine& machine = *controller->GetMachine(s_Layer);
	const SceneAnimation::LayerInfo* layerLive = !live ? nullptr : s_Layer < 0 ? static_cast<const SceneAnimation::LayerInfo*>(live) :
		s_Layer < (int)live->Layers.size() ? &live->Layers[s_Layer] : nullptr;
	if (ImGui::CollapsingHeader("States", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed |= StateControls(*controller, machine, model, layerLive);
	}
	if (ImGui::CollapsingHeader("Transitions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		changed |= TransitionControls(*controller, machine, layerLive);
	}
	if (changed)
	{
		controller->MarkChanged(); // the running game takes it at its next frame
	}
	ImGui::End();
}
