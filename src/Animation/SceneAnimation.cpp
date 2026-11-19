#include "SceneAnimation.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "H2M/Renderer/ModelH2M.h"

#include "Core/Log.h"

#include <algorithm>
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
	if (animator.Machines.empty() || animator.Machines.front().Current < 0)
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

	// The base and the layers: a layer kept by its name keeps its states
	std::vector<Machine> machines(controller.Layers.size() + 1);
	for (size_t m = 0; m < machines.size(); m++)
	{
		const std::string layer = m == 0 ? std::string() : controller.Layers[m - 1].Name;
		bool kept = false;
		for (Machine& old : animator.Machines)
		{
			if (old.Layer == layer && (m == 0 || !old.Layer.empty()))
			{
				machines[m] = std::move(old);
				old.Layer = "\x01"; // taken
				kept = true;
				break;
			}
		}
		machines[m].Layer = layer;
		const AnimatorStateMachine& states = m == 0 ? static_cast<const AnimatorStateMachine&>(controller) : controller.Layers[m - 1];
		BindMachine(animator, states, machines[m], rebinding && kept);
		skeleton.GetMask(m == 0 ? std::vector<std::string>() : controller.Layers[m - 1].Mask, machines[m].Mask);
		if (m > 0 && !controller.Layers[m - 1].Mask.empty())
		{
			for (const std::string& node : controller.Layers[m - 1].Mask)
			{
				if (skeleton.FindNode(node) < 0)
				{
					Log::GetLogger()->warn("Animator of '{0}': the mask of layer '{1}' names '{2}', which the model hasn't", name, layer, node);
				}
			}
		}
	}
	animator.Machines = std::move(machines);
	animator.Revision = controller.Revision;
}

void SceneAnimation::BindMachine(Animator& animator, const AnimatorStateMachine& states, Machine& machine, bool rebinding)
{
	const AnimationSkeleton& skeleton = *animator.Model->GetSkeleton();
	const std::string& name = m_Scene->Get<NameComponent>(animator.Entity).Name;
	const std::string where = machine.Layer.empty() ? std::string() : " (layer '" + machine.Layer + "')";

	// The states' clips: from the model's own file (its clips) or another file of the same rig
	auto bindClip = [&](const std::string& file, const std::string& clipName, const std::string& state) -> StateClip
	{
		StateClip clip;
		if (file.empty())
		{
			clip.Clip = AnimationClipLibrary::Find(animator.Model->GetClips(), clipName);
		}
		else
		{
			clip.File = AnimationClipLibrary::Load(file);
			clip.Clip = AnimationClipLibrary::Find(*clip.File, clipName);
		}
		if (!clip.Clip)
		{
			Log::GetLogger()->warn("Animator of '{0}'{1}: state '{2}' has no clip ({3}{4}): it shows the bind pose", name, where, state,
				file.empty() ? std::string("the model's own file") : file, clipName.empty() ? std::string() : ", '" + clipName + "'");
			return clip;
		}
		clip.Binding = AnimationBinding::Bind(skeleton, *clip.Clip);
		if (clip.Binding.BoundChannels == 0)
		{
			Log::GetLogger()->warn("Animator of '{0}'{1}: the clip of state '{2}' moves no node of the model (another rig?)", name, where, state);
		}
		return clip;
	};
	machine.Clips.assign(states.States.size(), std::vector<StateClip>());
	for (size_t s = 0; s < states.States.size(); s++)
	{
		const AnimatorState& state = states.States[s];
		if (state.Blend.empty())
		{
			machine.Clips[s].push_back(bindClip(state.ClipFile, state.ClipName, state.Name));
			continue;
		}
		for (const AnimatorBlendClip& clip : state.Blend)
		{
			machine.Clips[s].push_back(bindClip(clip.ClipFile, clip.ClipName, state.Name));
		}
	}

	// The states it was in, by name: gone ones end the crossfade, or start the entry state again
	if (rebinding)
	{
		auto rename = [&](int state) -> int
		{
			return state >= 0 && state < (int)machine.StateNames.size() ? states.FindState(machine.StateNames[state]) : -1;
		};
		const int current = rename(machine.Current);
		const int previous = rename(machine.Previous);
		if (machine.Fading && machine.Previous >= 0 && previous < 0)
		{
			machine.Fading = false;
		}
		machine.Previous = previous;
		if (current < 0)
		{
			machine.Current = states.GetEntryState();
			machine.CurrentTime = 0.0f;
			machine.Fading = false;
		}
		else
		{
			machine.Current = current;
		}
	}
	else
	{
		machine.Current = states.GetEntryState();
		machine.CurrentTime = 0.0f;
		machine.Previous = -1;
		machine.Fading = false;
	}
	machine.StateNames.clear();
	for (const AnimatorState& state : states.States)
	{
		machine.StateNames.push_back(state.Name);
	}
}

float SceneAnimation::GetStateSpeed(const Animator& animator, const AnimatorStateMachine& states, int state) const
{
	const AnimatorState& settings = states.States[state];
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

void SceneAnimation::GetBlend(const Animator& animator, const AnimatorState& state, size_t clips, size_t& a, size_t& b, float& weight) const
{
	a = b = 0;
	weight = 0.0f;
	const size_t count = std::min(clips, state.Blend.size());
	const int parameter = animator.Controller->FindParameter(state.BlendParameter);
	if (count < 2 || parameter < 0 || parameter >= (int)animator.Values.size())
	{
		return; // the first clip alone
	}
	const float value = animator.Values[parameter];
	if (value <= state.Blend.front().Threshold)
	{
		return;
	}
	if (value >= state.Blend[count - 1].Threshold)
	{
		a = b = count - 1;
		return;
	}
	for (size_t i = 0; i + 1 < count; i++)
	{
		const float low = state.Blend[i].Threshold, high = state.Blend[i + 1].Threshold;
		if (value >= low && value <= high)
		{
			a = i;
			b = i + 1;
			weight = high > low ? (value - low) / (high - low) : 0.0f;
			return;
		}
	}
}

float SceneAnimation::GetStateLength(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, int state) const
{
	if (state < 0 || state >= (int)machine.Clips.size() || machine.Clips[state].empty())
	{
		return 0.0f;
	}
	const std::vector<StateClip>& clips = machine.Clips[state];
	auto length = [&](size_t i) { return clips[i].Clip ? clips[i].Clip->GetDuration() : 0.0f; };
	if (clips.size() == 1 || states.States[state].Blend.empty())
	{
		return length(0);
	}
	size_t a, b;
	float weight;
	GetBlend(animator, states.States[state], clips.size(), a, b, weight);
	return length(a) + (length(b) - length(a)) * weight;
}

bool SceneAnimation::CanTake(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, const AnimatorTransition& transition) const
{
	const AnimatorController& controller = *animator.Controller;
	const int to = states.FindState(transition.To);
	if (to < 0)
	{
		return false;
	}
	if (transition.FromAny)
	{
		if (to == machine.Current)
		{
			return false; // Any State doesn't start the state it is in again
		}
	}
	else if (states.FindState(transition.From) != machine.Current)
	{
		return false;
	}

	if (transition.HasExitTime)
	{
		const float normalized = machine.CurrentTime;
		const bool loop = states.States[machine.Current].Loop;
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

void SceneAnimation::StartTransition(Animator& animator, Machine& machine, int to, float fade)
{
	if (fade <= 0.0f)
	{
		machine.Fading = false;
		machine.Previous = -1;
	}
	else
	{
		if (machine.Fading || machine.Current < 0)
		{
			// From what is shown now: a crossfade cut short, or nothing played yet (the bind pose)
			if (machine.Pose.Nodes.empty())
			{
				animator.Model->GetSkeleton()->GetBindPose(machine.Pose);
			}
			machine.Snapshot = machine.Pose;
			machine.Previous = -1;
		}
		else
		{
			machine.Previous = machine.Current;
			machine.PreviousTime = machine.CurrentTime;
		}
		machine.FadeDuration = fade;
		machine.FadeElapsed = 0.0f;
		machine.Fading = true;
	}
	machine.Current = to;
	machine.CurrentTime = 0.0f;
	animator.Transitions++;
}

void SceneAnimation::SampleState(const Animator& animator, const Machine& machine, const AnimatorStateMachine& states, int state, float time, AnimationPose& pose)
{
	const AnimationSkeleton& skeleton = *animator.Model->GetSkeleton();
	skeleton.GetBindPose(pose);
	if (state < 0 || state >= (int)machine.Clips.size() || machine.Clips[state].empty())
	{
		return;
	}
	const AnimatorState& settings = states.States[state];
	const std::vector<StateClip>& clips = machine.Clips[state];
	// Each clip at the same fraction of its length (time is normalized)
	auto sample = [&](size_t i, AnimationPose& into)
	{
		if (clips[i].Clip)
		{
			AnimationSampling::SampleClip(*clips[i].Clip, clips[i].Binding, time * clips[i].Clip->GetDuration(), settings.Loop, settings.InPlace, into);
		}
	};
	if (settings.Blend.empty() || clips.size() == 1)
	{
		sample(0, pose);
		return;
	}
	size_t a, b;
	float weight;
	GetBlend(animator, settings, clips.size(), a, b, weight);
	if (a == b || weight <= 0.0f)
	{
		sample(a, pose);
		return;
	}
	m_BlendA = pose;
	m_BlendB = pose;
	sample(a, m_BlendA);
	sample(b, m_BlendB);
	AnimationSampling::BlendPoses(m_BlendA, m_BlendB, weight, skeleton, pose);
}

void SceneAnimation::UpdateMachine(Animator& animator, Machine& machine, const AnimatorStateMachine& states, float frameTime)
{
	if (machine.Current < 0 || machine.Current >= (int)states.States.size())
	{
		machine.Current = states.GetEntryState();
		machine.CurrentTime = 0.0f;
		if (machine.Current < 0)
		{
			return; // no states
		}
	}

	// The clips move on by the frame (each state at its speed over its length: normalized), and the crossfade with them.
	// The events passed: the state that weighs more's (the one faded to, from halfway through the crossfade)
	auto advance = [&](int state, float& time)
	{
		const float length = GetStateLength(animator, machine, states, state);
		if (length <= 0.0f)
		{
			time = std::max(time, 1.0f); // a state without a clip is over at once
			return;
		}
		time += frameTime * GetStateSpeed(animator, states, state) / length;
	};
	const float currentTime = machine.CurrentTime;
	advance(machine.Current, machine.CurrentTime);
	bool currentFires = true;
	if (machine.Fading)
	{
		machine.FadeElapsed += frameTime;
		currentFires = machine.FadeDuration <= 0.0f || machine.FadeElapsed >= 0.5f * machine.FadeDuration;
		if (machine.Previous >= 0)
		{
			const float previousTime = machine.PreviousTime;
			advance(machine.Previous, machine.PreviousTime);
			if (!currentFires)
			{
				FireEvents(animator, states, machine.Previous, previousTime, machine.PreviousTime);
			}
		}
		if (machine.FadeElapsed >= machine.FadeDuration)
		{
			machine.Fading = false;
			machine.Previous = -1;
		}
	}
	if (currentFires)
	{
		FireEvents(animator, states, machine.Current, currentTime, machine.CurrentTime);
	}

	// The first transition that may be taken: from Any State first, then from the current state. Its triggers are used up
	const AnimatorController& controller = *animator.Controller;
	for (int pass = 0; pass < 2; pass++)
	{
		bool taken = false;
		for (const AnimatorTransition& transition : states.Transitions)
		{
			if (transition.FromAny != (pass == 0) || !CanTake(animator, machine, states, transition))
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
			StartTransition(animator, machine, states.FindState(transition.To), transition.Fade);
			taken = true;
			break;
		}
		if (taken)
		{
			break;
		}
	}

	// The pose: the current state's, faded in over what it came from
	SampleState(animator, machine, states, machine.Current, machine.CurrentTime, machine.To);
	if (machine.Fading)
	{
		if (machine.Previous >= 0)
		{
			SampleState(animator, machine, states, machine.Previous, machine.PreviousTime, machine.From);
		}
		const AnimationPose& from = machine.Previous >= 0 ? machine.From : machine.Snapshot;
		const float weight = machine.FadeDuration > 0.0f ? machine.FadeElapsed / machine.FadeDuration : 1.0f;
		AnimationSampling::BlendPoses(from, machine.To, weight, *animator.Model->GetSkeleton(), machine.Pose);
	}
	else
	{
		machine.Pose = machine.To;
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
		if (animator.Machines.empty() || controller.States.empty())
		{
			continue; // no states: the model keeps its own pose
		}

		// The base, then each layer over it by its weight on the nodes of its mask
		for (size_t m = 0; m < animator.Machines.size(); m++)
		{
			Machine& machine = animator.Machines[m];
			const AnimatorStateMachine& states = m == 0 ? static_cast<const AnimatorStateMachine&>(controller) : controller.Layers[m - 1];
			if (m > 0)
			{
				const AnimatorLayer& layer = controller.Layers[m - 1];
				machine.Weight = layer.Weight;
				const int parameter = layer.WeightParameter.empty() ? -1 : controller.FindParameter(layer.WeightParameter);
				if (parameter >= 0 && parameter < (int)animator.Values.size() && controller.Parameters[parameter].Type == AnimatorParameterType::Float)
				{
					machine.Weight *= animator.Values[parameter];
				}
				machine.Weight = std::clamp(machine.Weight, 0.0f, 1.0f);
			}
			UpdateMachine(animator, machine, states, frameTime);
			if (m == 0)
			{
				animator.Pose = machine.Pose;
			}
			else if (machine.Current >= 0 && machine.Weight > 0.0f)
			{
				AnimationSampling::BlendPosesMasked(animator.Pose, machine.Pose, machine.Weight, machine.Mask, *animator.Model->GetSkeleton());
			}
		}
		animator.Model->SetPose(animator.Pose);
	}
	UpdateAttachments();
}

const AnimationPose* SceneAnimation::GetPose(EnvMapVulkanEntityID entity) const
{
	auto it = m_Animators.find(entity);
	return it != m_Animators.end() && it->second.Model && !it->second.Pose.Nodes.empty() ? &it->second.Pose : nullptr;
}

void SceneAnimation::UpdateAttachments()
{
	static const AnimationPose bindPose; // empty: the skeleton's bind places
	m_Scene->Each<BoneAttachmentComponent>([&](EnvMapVulkanEntityID entity, BoneAttachmentComponent& attachment)
	{
		attachment.BoneTransform = glm::mat4(1.0f);
		const EnvMapVulkanEntityID parent = m_Scene->GetParent(entity);
		const ModelComponent* model = parent != NoEntity ? m_Scene->TryGet<ModelComponent>(parent) : nullptr;
		const AnimationSkeleton* skeleton = model && model->Model ? model->Model->GetSkeleton() : nullptr;
		const int node = skeleton ? skeleton->FindNode(attachment.Bone) : -1;
		if (node >= 0)
		{
			const AnimationPose* pose = GetPose(parent);
			attachment.BoneTransform = skeleton->GetNodeTransform(pose ? *pose : bindPose, node);
		}
	});
}

bool SceneAnimation::GetNodeTransform(EnvMapVulkanEntityID entity, const std::string& node, glm::mat4& world) const
{
	if (!m_Scene || !m_Scene->Exists(entity))
	{
		return false;
	}
	const ModelComponent* model = m_Scene->TryGet<ModelComponent>(entity);
	const AnimationSkeleton* skeleton = model && model->Model ? model->Model->GetSkeleton() : nullptr;
	const int index = skeleton ? skeleton->FindNode(node) : -1;
	if (index < 0)
	{
		return false;
	}
	static const AnimationPose bindPose;
	const AnimationPose* pose = GetPose(entity);
	world = m_Scene->GetWorldTransform(entity) * skeleton->GetNodeTransform(pose ? *pose : bindPose, index);
	return true;
}

bool SceneAnimation::GetBindNodeTransform(const EnvMapVulkanScene& scene, EnvMapVulkanEntityID entity, const std::string& node, glm::mat4& world)
{
	const ModelComponent* model = scene.TryGet<ModelComponent>(entity);
	const AnimationSkeleton* skeleton = model && model->Model ? model->Model->GetSkeleton() : nullptr;
	const int index = skeleton ? skeleton->FindNode(node) : -1;
	if (index < 0)
	{
		return false;
	}
	world = scene.GetWorldTransform(entity) * skeleton->GetNodeTransform(AnimationPose(), index);
	return true;
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

SceneAnimation::Result SceneAnimation::Play(EnvMapVulkanEntityID entity, const std::string& state, float fade, const std::string& layer)
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
	const int index = layer.empty() ? -1 : animator.Controller->FindLayer(layer);
	const AnimatorStateMachine* states = animator.Controller->GetMachine(index);
	if ((!layer.empty() && index < 0) || !states || index + 1 >= (int)animator.Machines.size())
	{
		return Result::NoLayer;
	}
	const int to = states->FindState(state);
	if (to < 0)
	{
		return Result::NoState;
	}
	StartTransition(animator, animator.Machines[index + 1], to, fade);
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
	for (size_t m = 0; m < animator.Machines.size() && animator.Controller; m++)
	{
		const Machine& machine = animator.Machines[m];
		const AnimatorStateMachine* states = animator.Controller->GetMachine((int)m - 1);
		LayerInfo layer;
		auto stateName = [&](int state)
		{
			return state >= 0 && state < (int)machine.StateNames.size() ? machine.StateNames[state] : std::string();
		};
		layer.Name = machine.Layer;
		layer.State = stateName(machine.Current);
		layer.NormalizedTime = machine.CurrentTime;
		layer.StateTime = states && animator.Revision == animator.Controller->Revision ?
			machine.CurrentTime * GetStateLength(animator, machine, *states, machine.Current) : 0.0f;
		layer.InTransition = machine.Fading;
		layer.PreviousState = machine.Fading ? stateName(machine.Previous) : std::string();
		layer.FadeProgress = machine.Fading && machine.FadeDuration > 0.0f ? machine.FadeElapsed / machine.FadeDuration : 0.0f;
		layer.Weight = m == 0 ? 1.0f : machine.Weight;
		if (m == 0)
		{
			static_cast<LayerInfo&>(info) = layer;
		}
		else
		{
			info.Layers.push_back(layer);
		}
	}
	return true;
}

std::shared_ptr<AnimatorController> SceneAnimation::GetController(EnvMapVulkanEntityID entity) const
{
	auto it = m_Animators.find(entity);
	return it != m_Animators.end() ? it->second.Controller : nullptr;
}

void SceneAnimation::FireEvents(Animator& animator, const AnimatorStateMachine& states, int state, float from, float to)
{
	if (state < 0 || state >= (int)states.States.size() || from == to)
	{
		return;
	}
	const AnimatorState& settings = states.States[state];
	if (settings.Events.empty())
	{
		return;
	}
	std::vector<size_t>& passed = m_PassedEvents;
	AnimatorController::GetPassedEvents(settings, from, to, passed);
	for (size_t e : passed)
	{
		const AnimatorEvent& event = settings.Events[e];
		m_Events.push_back({ animator.Entity, event.Name, settings.Name });
		animator.Events++;
		animator.LastEvent = event.Name + " (" + settings.Name + ")";
	}
}
