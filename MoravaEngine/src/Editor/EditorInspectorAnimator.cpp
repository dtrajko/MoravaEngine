// The Inspector's Animator section (docs/engine/Game_Engine_Runtime_Guide.html, 6.4): a skinned model's animator controller
// (a .manim file: chosen, dropped from the Content Browser, or made new from the model's clips) and, while the game runs,
// what its animator does: the state, the crossfade, and the parameters, which can be set here as a script would. The
// controller itself is edited in the Animator panel.


#include "Animation/AnimatorController.h"
#include "Animation/SceneAnimation.h"

#include "H2M/Renderer/ModelH2M.h"

#include "Project/Project.h"

#include "Core/FileDialogs.h"
#include "Core/Util.h"

#include "imgui.h"

#include <cstdio>
#include <filesystem>
#include <string>


#include "EditorState.h"


static bool IsControllerFile(const std::filesystem::path& path)
{
	return path.extension().u8string() == AnimatorController::FileExtension;
}

static bool ControllerControls(AnimatorComponent& animator, EnvMapVulkanEntityID entity, const H2M::RefH2M<H2M::ModelH2M>& model)
{
	bool changed = false;
	const std::string shown = animator.Controller.empty() ? std::string("(none: choose or make a .manim)") : ProjectPaths::ToStored(animator.Controller);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::SetNextItemWidth(-130.0f);
	ImGui::BeginDisabled(true);
	ImGui::InputText("##Controller", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			const std::filesystem::path path = std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data));
			if (IsControllerFile(path))
			{
				animator.Controller = ProjectPaths::Normalize(path.lexically_normal().generic_u8string());
				changed = true;
			}
			else
			{
				ShowNotification("An animator controller is a .manim file", true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s\nThe states, transitions and parameters the model is animated by. Drop a .manim file from the\n"
			"Content Browser here, choose one with ..., or make one from the model's clips with New",
			animator.Controller.empty() ? "No controller yet" : animator.Controller.c_str());
	}
	ImGui::SameLine();
	if (ImGui::Button("...##ChooseController"))
	{
		const std::string file = FileDialogs::OpenFile({ { "Animator controller", "manim" } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			animator.Controller = ProjectPaths::Normalize(std::filesystem::u8path(file).lexically_normal().generic_u8string());
			changed = true;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("New##NewController"))
	{
		const std::string file = CreateAnimatorController(model, s_Scene.Get<NameComponent>(entity).Name);
		if (!file.empty())
		{
			animator.Controller = ProjectPaths::Normalize(file);
			changed = true;
			OpenAnimatorController(animator.Controller);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A new controller with a state for each of the model's clips (in the project's Assets/Animators),\n"
			"opened in the Animator panel");
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(animator.Controller.empty());
	if (ImGui::Button("Edit##EditController"))
	{
		OpenAnimatorController(animator.Controller);
	}
	ImGui::EndDisabled();
	return changed;
}

// The running animator: its state, crossfade and parameters (set here as a script would)
static void LiveControls(EnvMapVulkanEntityID entity, SceneAnimation& game)
{
	SceneAnimation::Info info;
	if (!game.GetInfo(entity, info))
	{
		ImGui::TextDisabled("Not in the running game (added after it started)");
		return;
	}
	if (!info.Problem.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "It doesn't animate: %s", info.Problem.c_str());
		return;
	}
	ImGui::Text("State: %s  (%.2f s, %.2f of its clip)", info.State.c_str(), info.StateTime, info.NormalizedTime);
	if (info.InTransition)
	{
		const std::string from = info.PreviousState.empty() ? std::string("the pose before") : info.PreviousState;
		ImGui::Text("From %s", from.c_str());
		ImGui::SameLine();
		char overlay[32];
		snprintf(overlay, sizeof(overlay), "%.0f%%", info.FadeProgress * 100.0f);
		ImGui::ProgressBar(info.FadeProgress, ImVec2(-1.0f, 0.0f), overlay);
	}
	ImGui::TextDisabled("%llu transition(s) taken", (unsigned long long)info.Transitions);
	if (info.Events > 0)
	{
		ImGui::TextDisabled("%llu event(s), the last: %s", (unsigned long long)info.Events, info.LastEvent.c_str());
	}

	const std::shared_ptr<AnimatorController> controller = game.GetController(entity);
	if (!controller)
	{
		return;
	}
	for (size_t p = 0; p < controller->Parameters.size() && p < info.Values.size(); p++)
	{
		const AnimatorParameter& parameter = controller->Parameters[p];
		ImGui::PushID((int)p);
		float value = info.Values[p];
		switch (parameter.Type)
		{
			case AnimatorParameterType::Float:
				if (ImGui::DragFloat(parameter.Name.c_str(), &value, 0.01f))
				{
					game.SetParameter(entity, parameter.Name, parameter.Type, value);
				}
				break;
			case AnimatorParameterType::Bool:
			{
				bool on = value != 0.0f;
				if (ImGui::Checkbox(parameter.Name.c_str(), &on))
				{
					game.SetParameter(entity, parameter.Name, parameter.Type, on ? 1.0f : 0.0f);
				}
				break;
			}
			case AnimatorParameterType::Trigger:
				if (ImGui::SmallButton("Set"))
				{
					game.SetParameter(entity, parameter.Name, parameter.Type, 1.0f);
				}
				ImGui::SameLine();
				ImGui::Text("%s%s", parameter.Name.c_str(), value != 0.0f ? " (set)" : "");
				break;
		}
		ImGui::PopID();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The parameters as the game has them now: set here as a script would (the script may set them again)");
	}
}

void AnimatorControls(EnvMapVulkanEntityID entity)
{
	const H2M::RefH2M<H2M::ModelH2M> model = s_Scene.Has<ModelComponent>(entity) ? s_Scene.Get<ModelComponent>(entity).Model : H2M::RefH2M<H2M::ModelH2M>();
	const bool animated = model && model->IsSkinned() && model->GetSkeleton();
	if (!animated && !s_Scene.Has<AnimatorComponent>(entity))
	{
		return; // only skinned models are animated
	}

	ImGui::Separator();
	ImGui::PushID("Animator");
	bool hasAnimator = s_Scene.Has<AnimatorComponent>(entity);
	ImGui::BeginDisabled(IsPlaying());
	if (ImGui::Checkbox("Animator", &hasAnimator))
	{
		if (hasAnimator)
		{
			s_Scene.Add<AnimatorComponent>(entity);
		}
		else
		{
			s_Scene.Remove<AnimatorComponent>(entity);
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(IsPlaying() ? "Added and removed while the game doesn't run" :
			"While the game runs, a controller's states pose the model: the scripts set its parameters\n"
			"(GetComponent<Animator>().SetFloat(\"Speed\", ...)), its transitions crossfade between the clips");
	}
	AnimatorComponent* animator = s_Scene.TryGet<AnimatorComponent>(entity);
	if (!animator)
	{
		ImGui::PopID();
		return;
	}
	if (!animated)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "The model isn't skinned: nothing to animate");
	}

	ImGui::BeginDisabled(IsPlaying());
	ControllerControls(*animator, entity, model);
	ImGui::EndDisabled();

	if (SceneAnimation* game = GetGameAnimation())
	{
		LiveControls(entity, *game);
	}
	else
	{
		ImGui::TextDisabled("In Play and Simulate it poses the model; here the model plays its own clip (Animation)");
	}
	ImGui::PopID();
}
