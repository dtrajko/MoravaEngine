#include "SceneAnimation.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "H2M/Renderer/ModelH2M.h"

#include "Core/Log.h"

#include <cmath>


void SceneAnimation::Start(EnvMapVulkanScene& scene)
{
	Stop();
	m_Scene = &scene;
	std::vector<EnvMapVulkanEntityID> entities;
	scene.Each<AnimatorComponent>([&](EnvMapVulkanEntityID entity, AnimatorComponent&) { entities.push_back(entity); });
	AddEntities(entities);
	if (!m_Animators.empty())
	{
		uint32_t animating = 0;
		for (const auto& [entity, animator] : m_Animators)
		{
			animating += animator.Problem.empty() ? 1 : 0;
		}
		Log::GetLogger()->info("Animation: {0} animator(s), {1} animating", m_Animators.size(), animating);
	}
}

void SceneAnimation::AddEntities(const std::vector<EnvMapVulkanEntityID>& entities)
{
	if (!m_Scene)
	{
		return;
	}
	for (EnvMapVulkanEntityID entity : entities)
	{
		if (m_Scene->Has<AnimatorComponent>(entity) && m_Animators.find(entity) == m_Animators.end())
		{
			AddEntity(entity);
		}
	}
}

void SceneAnimation::Sync()
{
	if (!m_Scene)
	{
		return;
	}
	std::vector<EnvMapVulkanEntityID> entities;
	for (auto it = m_Animators.begin(); it != m_Animators.end();)
	{
		Animator& animator = it->second;
		const AnimatorComponent* component = m_Scene->Exists(animator.Entity) ? m_Scene->TryGet<AnimatorComponent>(animator.Entity) : nullptr;
		const ModelComponent* model = component ? m_Scene->TryGet<ModelComponent>(animator.Entity) : nullptr;
		if (component && component->Controller == animator.ControllerFile && (model ? model->Model.Raw() : nullptr) == animator.SourceModel)
		{
			++it;
			continue;
		}
		if (animator.Model)
		{
			animator.Model->ReleasePose(); // back to its own playback
		}
		it = m_Animators.erase(it); // another controller or model: added again below, in its entry state
	}
	m_Scene->Each<AnimatorComponent>([&](EnvMapVulkanEntityID entity, AnimatorComponent&) { entities.push_back(entity); });
	AddEntities(entities);
}

void SceneAnimation::AddEntity(EnvMapVulkanEntityID entity)
{
	Animator& animator = m_Animators[entity];
	animator.Entity = entity;
	const AnimatorComponent& component = m_Scene->Get<AnimatorComponent>(entity);
	animator.ControllerFile = component.Controller;
	const std::string& name = m_Scene->Get<NameComponent>(entity).Name;

	const ModelComponent* model = m_Scene->TryGet<ModelComponent>(entity);
	animator.SourceModel = model ? model->Model.Raw() : nullptr;
	if (!model || !model->Model || !model->Model->GetSkeleton() || !model->Model->IsSkinned())
	{
		animator.Problem = "the entity has no animated (skinned) model";
		Log::GetLogger()->warn("Animator of '{0}': {1}, so it doesn't animate", name, animator.Problem);
		return;
	}
	animator.Model = model->Model;
	if (component.Controller.empty())
	{
		animator.Problem = "no controller (.manim) chosen";
		Log::GetLogger()->warn("Animator of '{0}': {1}", name, animator.Problem);
		return;
	}
	animator.Controller = AnimatorControllerLibrary::Get(component.Controller);
	if (!animator.Controller)
	{
		animator.Problem = "its controller can't be read (the log says why)";
		return;
	}
	Bind(animator);
	animator.Current = animator.Controller->GetEntryState();
	if (animator.Current < 0)
	{
		Log::GetLogger()->warn("Animator of '{0}': its controller '{1}' has no states", name, component.Controller);
	}
}

void SceneAnimation::Bind(Animator& animator)
{
	const AnimatorController& controller = *animator.Controller;
	const AnimationSkeleton& skeleton = *animator.Model->GetSkeleton();
	const std::string& name = m_Scene->Get<NameComponent>(animator.Entity).Name;
	const bool rebinding = animator.Revision != 0;

	// The parameters: a value set before an edit stays, by name (a parameter that changed its type starts again)
	std::vector<float> values(controller.Parameters.size());
	for (size_t p = 0; p < controller.Parameters.size(); p++)
	{
		const AnimatorParameter& parameter = controller.Parameters[p];
		values[p] = parameter.Type == AnimatorParameterType::Trigger ? 0.0f : parameter.Default;
		for (size_t old = 0; rebinding && old < animator.ParameterNames.size(); old++)
		{
			if (animator.ParameterNames[old] == parameter.Name && old < animator.Values.size())
			{
				values[p] = animator.Values[old];
			}
		}
	}
	animator.Values = std::move(values);
	animator.ParameterNames.clear();
	for (const AnimatorParameter& parameter : controller.Parameters)
	{
		animator.ParameterNames.push_back(parameter.Name);
	}

	// The states' clips: from the model's own file (its clips) or another file of the same rig
	animator.Clips.assign(controller.States.size(), StateClip());
	for (size_t s = 0; s < controller.States.size(); s++)
	{
		const AnimatorState& state = controller.States[s];
		StateClip& clip = animator.Clips[s];
		if (state.ClipFile.empty())
		{
			clip.Clip = AnimationClipLibrary::Find(animator.Model->GetClips(), state.ClipName);
		}
		else
		{
			clip.File = AnimationClipLibrary::Load(state.ClipFile);
			clip.Clip = AnimationClipLibrary::Find(*clip.File, state.ClipName);
		}
		if (!clip.Clip)
		{
			Log::GetLogger()->warn("Animator of '{0}': state '{1}' has no clip ({2}{3}): it shows the bind pose", name, state.Name,
				state.ClipFile.empty() ? std::string("the model's own file") : state.ClipFile,
				state.ClipName.empty() ? std::string() : ", '" + state.ClipName + "'");
			continue;
		}
		clip.Binding = AnimationBinding::Bind(skeleton, *clip.Clip);
		if (clip.Binding.BoundChannels == 0)
		{
			Log::GetLogger()->warn("Animator of '{0}': the clip of state '{1}' moves no node of the model (another rig?)", name, state.Name);
		}
	}

	// The states it was in, by name: gone ones end the crossfade, or start the entry state again
	if (rebinding)
	{
		auto rename = [&](int state) -> int
		{
			return state >= 0 && state < (int)animator.StateNames.size() ? controller.FindState(animator.StateNames[state]) : -1;
		};
		const int current = rename(animator.Current);
		const int previous = rename(animator.Previous);
		if (animator.Fading && animator.Previous >= 0 && previous < 0)
		{
			animator.Fading = false;
		}
		animator.Previous = previous;
		if (current < 0)
		{
			animator.Current = controller.GetEntryState();
			animator.CurrentTime = 0.0f;
			animator.Fading = false;
		}
		else
		{
			animator.Current = current;
		}
	}
	animator.StateNames.clear();
	for (const AnimatorState& state : controller.States)
	{
		animator.StateNames.push_back(state.Name);
	}
	animator.Revision = controller.Revision;
}

float SceneAnimation::GetStateSpeed(const Animator& animator, int state) const
{
	const AnimatorState& settings = animator.Controller->States[state];
	float speed = settings.Speed;
	if (!settings.SpeedParameter.empty())
	{
		const int parameter = animator.Controller->FindParameter(settings.SpeedParameter);
		if (parameter >= 0 && parameter < (int)animator.Values.size() && animator.Controller->Parameters[parameter].Type == AnimatorParameterType::Float)
		{
			speed *= animator.Values[parameter];
		}
	}
	return speed;
}

float SceneAnimation::GetNormalizedTime(const Animator& animator, int state, float time) const
{
	const AnimationClip* clip = state >= 0 && state < (int)animator.Clips.size() ? animator.Clips[state].Clip : nullptr;
	return clip && clip->GetDuration() > 0.0f ? time / clip->GetDuration() : 1.0f; // a state without a clip is over at once
}

bool SceneAnimation::CanTake(const Animator& animator, const AnimatorTransition& transition) const
{
	const AnimatorController& controller = *animator.Controller;
	const int to = controller.FindState(transition.To);
	if (to < 0)
	{
		return false;
	}
	if (transition.FromAny)
	{
		if (to == animator.Current)
		{
			return false; // Any State doesn't start the state it is in again
		}
	}
	else if (controller.FindState(transition.From) != animator.Current)
	{
		return false;
	}

	if (transition.HasExitTime)
	{
		const float normalized = GetNormalizedTime(animator, animator.Current, animator.CurrentTime);
		const bool loop = controller.States[animator.Current].Loop;
		const bool reached = loop && transition.ExitTime < 1.0f ? normalized - std::floor(normalized) >= transition.ExitTime
			: normalized >= transition.ExitTime;
		if (!reached)
		{
			return false;
		}
	}

	for (const AnimatorCondition& condition : transition.Conditions)
	{
		const int parameter = controller.FindParameter(condition.Parameter);
		if (parameter < 0 || parameter >= (int)animator.Values.size())
		{
			return false;
		}
		const float value = animator.Values[parameter];
		switch (controller.Parameters[parameter].Type)
		{
			case AnimatorParameterType::Float:
				if (condition.Test == AnimatorConditionTest::Greater ? !(value > condition.Value) : !(value < condition.Value))
				{
					return false;
				}
				break;
			case AnimatorParameterType::Bool:
				if ((value != 0.0f) != (condition.Test != AnimatorConditionTest::False))
				{
					return false;
				}
				break;
			case AnimatorParameterType::Trigger:
				if (value == 0.0f)
				{
					return false;
				}
				break;
		}
	}
	return true;
}

void SceneAnimation::StartTransition(Animator& animator, int to, float fade)
{
	if (fade <= 0.0f)
	{
		animator.Fading = false;
		animator.Previous = -1;
	}
	else
	{
		if (animator.Fading || animator.Current < 0)
		{
			// From what is shown now: a crossfade cut short, or nothing played yet (the bind pose)
			if (animator.Pose.Nodes.empty())
			{
				animator.Model->GetSkeleton()->GetBindPose(animator.Pose);
			}
			animator.Snapshot = animator.Pose;
			animator.Previous = -1;
		}
		else
		{
			animator.Previous = animator.Current;
			animator.PreviousTime = animator.CurrentTime;
		}
		animator.FadeDuration = fade;
		animator.FadeElapsed = 0.0f;
		animator.Fading = true;
	}
	animator.Current = to;
	animator.CurrentTime = 0.0f;
	animator.Transitions++;
}

void SceneAnimation::SampleState(const Animator& animator, int state, float time, AnimationPose& pose) const
{
	animator.Model->GetSkeleton()->GetBindPose(pose);
	if (state < 0 || state >= (int)animator.Clips.size() || !animator.Clips[state].Clip)
	{
		return;
	}
	const AnimatorState& settings = animator.Controller->States[state];
	AnimationSampling::SampleClip(*animator.Clips[state].Clip, animator.Clips[state].Binding, time, settings.Loop, settings.InPlace, pose);
}

void SceneAnimation::FireEvents(Animator& animator, int state, float from, float to)
{
	if (state < 0 || state >= (int)animator.Clips.size() || from == to)
	{
		return;
	}
	const AnimatorState& settings = animator.Controller->States[state];
	if (settings.Events.empty())
	{
		return;
	}
	std::vector<size_t>& passed = m_PassedEvents;
	AnimatorController::GetPassedEvents(settings, GetNormalizedTime(animator, state, from), GetNormalizedTime(animator, state, to), passed);
	for (size_t e : passed)
	{
		const AnimatorEvent& event = settings.Events[e];
		m_Events.push_back({ animator.Entity, event.Name, settings.Name });
		animator.Events++;
		animator.LastEvent = event.Name + " (" + settings.Name + ")";
	}
}

void SceneAnimation::Update(float frameTime)
{
	m_Events.clear();
	if (!m_Scene)
	{
		return;
	}
	for (auto it = m_Animators.begin(); it != m_Animators.end();)
	{
		Animator& animator = it->second;
		if (!m_Scene->Exists(animator.Entity))
		{
			it = m_Animators.erase(it); // destroyed: its model goes with it
			continue;
		}
		++it;
		if (!animator.Model || !animator.Controller)
		{
			continue;
		}
		const AnimatorController& controller = *animator.Controller;
		if (animator.Revision != controller.Revision)
		{
			Bind(animator);
		}
		if (animator.Current < 0 || animator.Current >= (int)controller.States.size())
		{
			animator.Current = controller.GetEntryState();
			animator.CurrentTime = 0.0f;
			if (animator.Current < 0)
			{
				continue; // no states: the model keeps its own pose
			}
		}

		// The clips move on by the frame (each state at its speed), and the crossfade with them. The events passed: the
		// state that weighs more's (the one faded to, from halfway through the crossfade)
		const float currentTime = animator.CurrentTime;
		animator.CurrentTime += frameTime * GetStateSpeed(animator, animator.Current);
		bool currentFires = true;
		if (animator.Fading)
		{
			animator.FadeElapsed += frameTime;
			currentFires = animator.FadeDuration <= 0.0f || animator.FadeElapsed >= 0.5f * animator.FadeDuration;
			if (animator.Previous >= 0)
			{
				const float previousTime = animator.PreviousTime;
				animator.PreviousTime += frameTime * GetStateSpeed(animator, animator.Previous);
				if (!currentFires)
				{
					FireEvents(animator, animator.Previous, previousTime, animator.PreviousTime);
				}
			}
			if (animator.FadeElapsed >= animator.FadeDuration)
			{
				animator.Fading = false;
				animator.Previous = -1;
			}
		}
		if (currentFires)
		{
			FireEvents(animator, animator.Current, currentTime, animator.CurrentTime);
		}

		// The first transition that may be taken: from Any State first, then from the current state. Its triggers are used up
		for (int pass = 0; pass < 2; pass++)
		{
			bool taken = false;
			for (const AnimatorTransition& transition : controller.Transitions)
			{
				if (transition.FromAny != (pass == 0) || !CanTake(animator, transition))
				{
					continue;
				}
				for (const AnimatorCondition& condition : transition.Conditions)
				{
					const int parameter = controller.FindParameter(condition.Parameter);
					if (parameter >= 0 && controller.Parameters[parameter].Type == AnimatorParameterType::Trigger)
					{
						animator.Values[parameter] = 0.0f;
					}
				}
				StartTransition(animator, controller.FindState(transition.To), transition.Fade);
				taken = true;
				break;
			}
			if (taken)
			{
				break;
			}
		}

		// The pose: the current state's, faded in over what it came from
		SampleState(animator, animator.Current, animator.CurrentTime, animator.To);
		if (animator.Fading)
		{
			if (animator.Previous >= 0)
			{
				SampleState(animator, animator.Previous, animator.PreviousTime, animator.From);
			}
			const AnimationPose& from = animator.Previous >= 0 ? animator.From : animator.Snapshot;
			const float weight = animator.FadeDuration > 0.0f ? animator.FadeElapsed / animator.FadeDuration : 1.0f;
			AnimationSampling::BlendPoses(from, animator.To, weight, *animator.Model->GetSkeleton(), animator.Pose);
		}
		else
		{
			animator.Pose = animator.To;
		}
		animator.Model->SetPose(animator.Pose);
	}
}

void SceneAnimation::Stop()
{
	for (auto& [entity, animator] : m_Animators)
	{
		if (animator.Model)
		{
			animator.Model->ReleasePose(); // back to its own playback (the Inspector's Animation section)
		}
	}
	m_Animators.clear();
	m_Events.clear();
	m_Scene = nullptr;
}

SceneAnimation::Result SceneAnimation::SetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float value)
{
	auto it = m_Animators.find(entity);
	if (it == m_Animators.end())
	{
		return Result::NoAnimator;
	}
	Animator& animator = it->second;
	const int parameter = animator.Controller ? animator.Controller->FindParameter(name) : -1;
	if (parameter < 0 || animator.Controller->Parameters[parameter].Type != type)
	{
		return Result::NoParameter;
	}
	if (animator.Revision != animator.Controller->Revision)
	{
		Bind(animator); // the values in the edited controller's order
	}
	animator.Values[parameter] = type == AnimatorParameterType::Float ? value : (value != 0.0f ? 1.0f : 0.0f);
	return Result::Done;
}

SceneAnimation::Result SceneAnimation::GetParameter(EnvMapVulkanEntityID entity, const std::string& name, AnimatorParameterType type, float& value) const
{
	auto it = m_Animators.find(entity);
	if (it == m_Animators.end())
	{
		return Result::NoAnimator;
	}
	const Animator& animator = it->second;
	const int parameter = animator.Controller ? animator.Controller->FindParameter(name) : -1;
	if (parameter < 0 || animator.Controller->Parameters[parameter].Type != type)
	{
		return Result::NoParameter;
	}
	// Before the next frame binds an edited controller, the value is under its name as bound
	for (size_t p = 0; p < animator.ParameterNames.size() && p < animator.Values.size(); p++)
	{
		if (animator.ParameterNames[p] == name)
		{
			value = animator.Values[p];
			return Result::Done;
		}
	}
	value = animator.Controller->Parameters[parameter].Default;
	return Result::Done;
}

SceneAnimation::Result SceneAnimation::Play(EnvMapVulkanEntityID entity, const std::string& state, float fade)
{
	auto it = m_Animators.find(entity);
	if (it == m_Animators.end() || !it->second.Model || !it->second.Controller)
	{
		return Result::NoAnimator;
	}
	Animator& animator = it->second;
	if (animator.Revision != animator.Controller->Revision)
	{
		Bind(animator);
	}
	const int to = animator.Controller->FindState(state);
	if (to < 0)
	{
		return Result::NoState;
	}
	StartTransition(animator, to, fade);
	return Result::Done;
}

bool SceneAnimation::GetInfo(EnvMapVulkanEntityID entity, Info& info) const
{
	auto it = m_Animators.find(entity);
	if (it == m_Animators.end())
	{
		return false;
	}
	const Animator& animator = it->second;
	info = Info();
	info.Controller = animator.ControllerFile;
	info.Problem = animator.Problem;
	info.Transitions = animator.Transitions;
	info.Events = animator.Events;
	info.LastEvent = animator.LastEvent;
	info.Values = animator.Values;
	auto stateName = [&](int state)
	{
		return state >= 0 && state < (int)animator.StateNames.size() ? animator.StateNames[state] : std::string();
	};
	info.State = stateName(animator.Current);
	info.StateTime = animator.CurrentTime;
	info.NormalizedTime = animator.Controller ? GetNormalizedTime(animator, animator.Current, animator.CurrentTime) : 0.0f;
	info.InTransition = animator.Fading;
	info.PreviousState = animator.Fading ? stateName(animator.Previous) : std::string();
	info.FadeProgress = animator.Fading && animator.FadeDuration > 0.0f ? animator.FadeElapsed / animator.FadeDuration : 0.0f;
	return true;
}

std::shared_ptr<AnimatorController> SceneAnimation::GetController(EnvMapVulkanEntityID entity) const
{
	auto it = m_Animators.find(entity);
	return it != m_Animators.end() ? it->second.Controller : nullptr;
}
