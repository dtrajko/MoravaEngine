#include "AnimatorController.h"
#include "Assets/AssetFiles.h"

#include "Core/Log.h"

#include "Project/AssetRegistry.h"
#include "Project/Project.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <system_error>


int AnimatorStateMachine::FindState(const std::string& name) const
{
	for (size_t i = 0; i < States.size(); i++)
	{
		if (States[i].Name == name)
		{
			return (int)i;
		}
	}
	return -1;
}

int AnimatorStateMachine::GetEntryState() const
{
	const int entry = FindState(Entry);
	return entry >= 0 ? entry : (States.empty() ? -1 : 0);
}

int AnimatorController::FindLayer(const std::string& name) const
{
	for (size_t i = 0; i < Layers.size(); i++)
	{
		if (Layers[i].Name == name)
		{
			return (int)i;
		}
	}
	return -1;
}

const AnimatorStateMachine* AnimatorController::GetMachine(int layer) const
{
	if (layer < 0)
	{
		return this;
	}
	return layer < (int)Layers.size() ? &Layers[layer] : nullptr;
}

AnimatorStateMachine* AnimatorController::GetMachine(int layer)
{
	return const_cast<AnimatorStateMachine*>(static_cast<const AnimatorController*>(this)->GetMachine(layer));
}

int AnimatorController::FindParameter(const std::string& name) const
{
	for (size_t i = 0; i < Parameters.size(); i++)
	{
		if (Parameters[i].Name == name)
		{
			return (int)i;
		}
	}
	return -1;
}

void AnimatorController::SortEvents(AnimatorState& state)
{
	std::stable_sort(state.Events.begin(), state.Events.end(), [](const AnimatorEvent& a, const AnimatorEvent& b) { return a.Time < b.Time; });
}

void AnimatorController::SortBlend(AnimatorState& state)
{
	std::stable_sort(state.Blend.begin(), state.Blend.end(), [](const AnimatorBlendClip& a, const AnimatorBlendClip& b) { return a.Threshold < b.Threshold; });
}

void AnimatorController::GetPassedEvents(const AnimatorState& state, float from, float to, std::vector<size_t>& events)
{
	events.clear();
	if (state.Events.empty() || from == to)
	{
		return;
	}
	const bool forward = to > from;
	auto passed = [&](float at, float a, float b) { return forward ? at >= a && at < b : at > a && at <= b; };
	if (!state.Loop)
	{
		const float a = std::clamp(std::min(from, to), 0.0f, 1.0f), b = std::clamp(std::max(from, to), 0.0f, 1.0f);
		for (size_t e = 0; e < state.Events.size(); e++)
		{
			const float at = state.Events[e].Time;
			if (passed(at, a, b) || (forward && at >= a && b >= 1.0f && a < 1.0f))
			{
				events.push_back(e);
			}
		}
	}
	else
	{
		const float a = std::min(from, to), b = std::max(from, to);
		if (b - a >= 1.0f)
		{
			// A frame longer than a loop: each event once
			for (size_t e = 0; e < state.Events.size(); e++)
			{
				events.push_back(e);
			}
			return; // in time order either way
		}
		for (float loop = std::floor(a); loop <= std::floor(b); loop += 1.0f) // the frame's loop, and the next one
		{
			for (size_t e = 0; e < state.Events.size(); e++)
			{
				if (passed(loop + state.Events[e].Time, a, b))
				{
					events.push_back(e);
				}
			}
		}
	}
	if (!forward)
	{
		std::reverse(events.begin(), events.end()); // played backwards: the later ones first
	}
}

const char* AnimatorController::GetParameterTypeName(AnimatorParameterType type)
{
	switch (type)
	{
		case AnimatorParameterType::Float:   return "Float";
		case AnimatorParameterType::Bool:    return "Bool";
		case AnimatorParameterType::Trigger: return "Trigger";
	}
	return "Float";
}

const char* AnimatorController::GetConditionTestName(AnimatorConditionTest test)
{
	switch (test)
	{
		case AnimatorConditionTest::Greater: return "Greater";
		case AnimatorConditionTest::Less:    return "Less";
		case AnimatorConditionTest::True:    return "True";
		case AnimatorConditionTest::False:   return "False";
	}
	return "Greater";
}

// A state machine's states and transitions (the base's, or a layer's), as keys of the map being written
static void WriteMachine(YAML::Emitter& out, const AnimatorStateMachine& machine, const AnimatorController& controller)
{
	out << YAML::Key << "States" << YAML::Value << YAML::BeginSeq;
	for (const AnimatorState& state : machine.States)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << state.Name;
		if (state.Blend.empty())
		{
			out << YAML::Key << "Clip" << YAML::Value << YAML::Flow << YAML::BeginMap;
			AssetReference::Write(out, "File", state.ClipFile);
			out << YAML::Key << "Name" << YAML::Value << state.ClipName;
			out << YAML::EndMap;
		}
		else
		{
			out << YAML::Key << "BlendParameter" << YAML::Value << state.BlendParameter;
			out << YAML::Key << "Blend" << YAML::Value << YAML::BeginSeq;
			for (const AnimatorBlendClip& clip : state.Blend)
			{
				out << YAML::Flow << YAML::BeginMap;
				AssetReference::Write(out, "File", clip.ClipFile);
				out << YAML::Key << "Name" << YAML::Value << clip.ClipName;
				out << YAML::Key << "Threshold" << YAML::Value << clip.Threshold;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}
		out << YAML::Key << "Speed" << YAML::Value << state.Speed;
		if (!state.SpeedParameter.empty())
		{
			out << YAML::Key << "SpeedParameter" << YAML::Value << state.SpeedParameter;
		}
		out << YAML::Key << "Loop" << YAML::Value << state.Loop;
		out << YAML::Key << "InPlace" << YAML::Value << state.InPlace;
		if (!state.Events.empty())
		{
			out << YAML::Key << "Events" << YAML::Value << YAML::BeginSeq;
			for (const AnimatorEvent& event : state.Events)
			{
				out << YAML::Flow << YAML::BeginMap;
				out << YAML::Key << "Name" << YAML::Value << event.Name;
				out << YAML::Key << "Time" << YAML::Value << event.Time;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	out << YAML::Key << "Transitions" << YAML::Value << YAML::BeginSeq;
	for (const AnimatorTransition& transition : machine.Transitions)
	{
		out << YAML::BeginMap;
		if (transition.FromAny)
		{
			out << YAML::Key << "FromAny" << YAML::Value << true;
		}
		else
		{
			out << YAML::Key << "From" << YAML::Value << transition.From;
		}
		out << YAML::Key << "To" << YAML::Value << transition.To;
		out << YAML::Key << "Fade" << YAML::Value << transition.Fade;
		if (transition.HasExitTime)
		{
			out << YAML::Key << "ExitTime" << YAML::Value << transition.ExitTime;
		}
		out << YAML::Key << "Conditions" << YAML::Value << YAML::BeginSeq;
		for (const AnimatorCondition& condition : transition.Conditions)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << "Parameter" << YAML::Value << condition.Parameter;
			const int parameter = controller.FindParameter(condition.Parameter);
			const AnimatorParameterType type = parameter >= 0 ? controller.Parameters[parameter].Type : AnimatorParameterType::Float;
			if (type != AnimatorParameterType::Trigger)
			{
				out << YAML::Key << "Test" << YAML::Value << AnimatorController::GetConditionTestName(condition.Test);
			}
			if (type == AnimatorParameterType::Float)
			{
				out << YAML::Key << "Value" << YAML::Value << condition.Value;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;
}

std::string AnimatorController::ToText() const
{
	YAML::Emitter out;
	out << YAML::BeginMap;
	out << YAML::Key << "Animator" << YAML::Value << YAML::BeginMap;
	out << YAML::Key << "Version" << YAML::Value << Version;
	out << YAML::Key << "Entry" << YAML::Value << Entry;

	out << YAML::Key << "Parameters" << YAML::Value << YAML::BeginSeq;
	for (const AnimatorParameter& parameter : Parameters)
	{
		out << YAML::Flow << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << parameter.Name;
		out << YAML::Key << "Type" << YAML::Value << GetParameterTypeName(parameter.Type);
		if (parameter.Type == AnimatorParameterType::Float)
		{
			out << YAML::Key << "Default" << YAML::Value << parameter.Default;
		}
		else if (parameter.Type == AnimatorParameterType::Bool)
		{
			out << YAML::Key << "Default" << YAML::Value << (parameter.Default != 0.0f);
		}
		out << YAML::EndMap;
	}
	out << YAML::EndSeq;

	WriteMachine(out, *this, *this);

	if (!Layers.empty())
	{
		out << YAML::Key << "Layers" << YAML::Value << YAML::BeginSeq;
		for (const AnimatorLayer& layer : Layers)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << layer.Name;
			out << YAML::Key << "Mask" << YAML::Value << YAML::Flow << layer.Mask;
			out << YAML::Key << "Weight" << YAML::Value << layer.Weight;
			if (!layer.WeightParameter.empty())
			{
				out << YAML::Key << "WeightParameter" << YAML::Value << layer.WeightParameter;
			}
			out << YAML::Key << "Entry" << YAML::Value << layer.Entry;
			WriteMachine(out, layer, *this);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
	}

	out << YAML::EndMap;
	out << YAML::EndMap;
	return std::string(out.c_str()) + "\n";
}

static bool FindParameterType(const std::string& name, AnimatorParameterType& type)
{
	for (AnimatorParameterType candidate : { AnimatorParameterType::Float, AnimatorParameterType::Bool, AnimatorParameterType::Trigger })
	{
		if (name == AnimatorController::GetParameterTypeName(candidate))
		{
			type = candidate;
			return true;
		}
	}
	return false;
}

static bool FindConditionTest(const std::string& name, AnimatorConditionTest& test)
{
	for (AnimatorConditionTest candidate : { AnimatorConditionTest::Greater, AnimatorConditionTest::Less, AnimatorConditionTest::True,
		AnimatorConditionTest::False })
	{
		if (name == AnimatorController::GetConditionTestName(candidate))
		{
			test = candidate;
			return true;
		}
	}
	return false;
}

// A state machine's states and transitions (the base's, or a layer's: named in the warnings) from the map that has them
static void ReadMachine(const YAML::Node& node, const AnimatorController& controller, AnimatorStateMachine& machine,
	const std::string& filepath, const std::string& layer)
{
	const std::string where = layer.empty() ? std::string() : " (layer '" + layer + "')";
	if (const YAML::Node states = node["States"]; states && states.IsSequence())
	{
		for (const YAML::Node& stateNode : states)
		{
			AnimatorState state;
			state.Name = stateNode["Name"].as<std::string>("");
			if (state.Name.empty() || machine.FindState(state.Name) >= 0)
			{
				Log::GetLogger()->warn("Animator '{0}'{1}: a state without a name, or a second '{2}', is left out", filepath, where, state.Name);
				continue;
			}
			if (const YAML::Node clip = stateNode["Clip"]; clip && clip.IsMap())
			{
				state.ClipFile = AssetReference::Read(clip, "File");
				state.ClipName = clip["Name"].as<std::string>("");
			}
			if (const YAML::Node blend = stateNode["Blend"]; blend && blend.IsSequence())
			{
				for (const YAML::Node& clipNode : blend)
				{
					AnimatorBlendClip clip;
					clip.ClipFile = AssetReference::Read(clipNode, "File");
					clip.ClipName = clipNode["Name"].as<std::string>("");
					clip.Threshold = clipNode["Threshold"].as<float>(0.0f);
					state.Blend.push_back(std::move(clip));
				}
				AnimatorController::SortBlend(state);
				state.BlendParameter = stateNode["BlendParameter"].as<std::string>("");
				const int parameter = controller.FindParameter(state.BlendParameter);
				if (parameter < 0 || controller.Parameters[parameter].Type != AnimatorParameterType::Float)
				{
					Log::GetLogger()->warn("Animator '{0}'{1}: the blend state '{2}' is blended by '{3}', which isn't a Float parameter: it shows its first clip",
						filepath, where, state.Name, state.BlendParameter);
				}
			}
			state.Speed = stateNode["Speed"].as<float>(state.Speed);
			state.SpeedParameter = stateNode["SpeedParameter"].as<std::string>("");
			state.Loop = stateNode["Loop"].as<bool>(state.Loop);
			state.InPlace = stateNode["InPlace"].as<bool>(state.InPlace);
			if (const YAML::Node events = stateNode["Events"]; events && events.IsSequence())
			{
				for (const YAML::Node& eventNode : events)
				{
					AnimatorEvent event;
					event.Name = eventNode["Name"].as<std::string>("");
					event.Time = std::clamp(eventNode["Time"].as<float>(0.0f), 0.0f, 1.0f);
					if (event.Name.empty())
					{
						Log::GetLogger()->warn("Animator '{0}'{1}: an event of state '{2}' has no name: left out", filepath, where, state.Name);
						continue;
					}
					state.Events.push_back(std::move(event));
				}
				AnimatorController::SortEvents(state);
			}
			machine.States.push_back(std::move(state));
		}
	}
	if (const YAML::Node transitions = node["Transitions"]; transitions && transitions.IsSequence())
	{
		for (const YAML::Node& transitionNode : transitions)
		{
			AnimatorTransition transition;
			transition.FromAny = transitionNode["FromAny"].as<bool>(false);
			transition.From = transitionNode["From"].as<std::string>("");
			transition.To = transitionNode["To"].as<std::string>("");
			transition.Fade = std::max(transitionNode["Fade"].as<float>(transition.Fade), 0.0f);
			if (const YAML::Node exitTime = transitionNode["ExitTime"])
			{
				transition.HasExitTime = true;
				transition.ExitTime = std::max(exitTime.as<float>(1.0f), 0.0f);
			}
			if (const YAML::Node conditions = transitionNode["Conditions"]; conditions && conditions.IsSequence())
			{
				for (const YAML::Node& conditionNode : conditions)
				{
					AnimatorCondition condition;
					condition.Parameter = conditionNode["Parameter"].as<std::string>("");
					const std::string test = conditionNode["Test"].as<std::string>("");
					const int parameter = controller.FindParameter(condition.Parameter);
					if (parameter < 0)
					{
						Log::GetLogger()->warn("Animator '{0}'{1}: a condition of the transition to '{2}' tests '{3}', which isn't a parameter: left out",
							filepath, where, transition.To, condition.Parameter);
						continue;
					}
					const AnimatorParameterType type = controller.Parameters[parameter].Type;
					condition.Test = type == AnimatorParameterType::Bool ? AnimatorConditionTest::True : AnimatorConditionTest::Greater;
					if (!test.empty() && !FindConditionTest(test, condition.Test))
					{
						Log::GetLogger()->warn("Animator '{0}'{1}: a condition on '{2}' has the test '{3}' (Greater, Less, True or False)",
							filepath, where, condition.Parameter, test);
					}
					condition.Value = conditionNode["Value"].as<float>(0.0f);
					transition.Conditions.push_back(std::move(condition));
				}
			}
			if (machine.FindState(transition.To) < 0 || (!transition.FromAny && machine.FindState(transition.From) < 0))
			{
				Log::GetLogger()->warn("Animator '{0}'{1}: the transition from '{2}' to '{3}' names a state it doesn't have: left out",
					filepath, where, transition.FromAny ? std::string("Any State") : transition.From, transition.To);
				continue;
			}
			machine.Transitions.push_back(std::move(transition));
		}
	}
}

bool AnimatorController::FromText(const std::string& text, const std::string& filepath, AnimatorController& controller, std::string& error)
{
	YAML::Node root;
	try
	{
		root = YAML::Load(text);
	}
	catch (const std::exception& e)
	{
		error = std::string("it can't be read (") + e.what() + ")";
		return false;
	}
	const YAML::Node animator = root["Animator"];
	if (!animator || !animator.IsMap())
	{
		error = "it is not an animator controller (it has no Animator)";
		return false;
	}

	AnimatorController result;
	try
	{
		result.Entry = animator["Entry"].as<std::string>("");
		if (const YAML::Node parameters = animator["Parameters"]; parameters && parameters.IsSequence())
		{
			for (const YAML::Node& node : parameters)
			{
				AnimatorParameter parameter;
				parameter.Name = node["Name"].as<std::string>("");
				const std::string type = node["Type"].as<std::string>("Float");
				if (parameter.Name.empty() || result.FindParameter(parameter.Name) >= 0)
				{
					Log::GetLogger()->warn("Animator '{0}': a parameter without a name, or a second '{1}', is left out", filepath, parameter.Name);
					continue;
				}
				if (!FindParameterType(type, parameter.Type))
				{
					Log::GetLogger()->warn("Animator '{0}': parameter '{1}' has the type '{2}' (Float, Bool or Trigger): a Float then",
						filepath, parameter.Name, type);
				}
				if (parameter.Type == AnimatorParameterType::Bool)
				{
					parameter.Default = node["Default"].as<bool>(false) ? 1.0f : 0.0f;
				}
				else if (parameter.Type == AnimatorParameterType::Float)
				{
					parameter.Default = node["Default"].as<float>(0.0f);
				}
				result.Parameters.push_back(std::move(parameter));
			}
		}
		ReadMachine(animator, result, result, filepath, std::string());
		if (const YAML::Node layers = animator["Layers"]; layers && layers.IsSequence())
		{
			for (const YAML::Node& node : layers)
			{
				AnimatorLayer layer;
				layer.Name = node["Name"].as<std::string>("");
				if (layer.Name.empty() || result.FindLayer(layer.Name) >= 0)
				{
					Log::GetLogger()->warn("Animator '{0}': a layer without a name, or a second '{1}', is left out", filepath, layer.Name);
					continue;
				}
				if (const YAML::Node mask = node["Mask"]; mask && mask.IsSequence())
				{
					for (const YAML::Node& name : mask)
					{
						layer.Mask.push_back(name.as<std::string>(""));
					}
				}
				layer.Weight = std::clamp(node["Weight"].as<float>(1.0f), 0.0f, 1.0f);
				layer.WeightParameter = node["WeightParameter"].as<std::string>("");
				layer.Entry = node["Entry"].as<std::string>("");
				ReadMachine(node, result, layer, filepath, layer.Name);
				result.Layers.push_back(std::move(layer));
			}
		}
	}
	catch (const std::exception& e)
	{
		error = std::string("it can't be read (") + e.what() + ")";
		return false;
	}
	result.Revision = controller.Revision + 1; // a controller read again is a change to whoever plays it
	controller = std::move(result);
	return true;
}

bool AnimatorController::Load(const std::string& filepath, AnimatorController& controller, std::string& error)
{
	std::string text;
	if (!AssetFiles::ReadText(filepath, text))
	{
		error = "it can't be opened";
		return false;
	}
	return FromText(text, filepath, controller, error);
}

bool AnimatorController::Save(const std::string& filepath, std::string& error) const
{
	std::ofstream file(std::filesystem::u8path(filepath));
	if (!file)
	{
		error = "'" + filepath + "' can't be written";
		return false;
	}
	file << ToText();
	return true;
}

namespace AnimatorControllerLibrary
{
	static std::map<std::string, std::shared_ptr<AnimatorController>> s_Controllers;

	static std::string GetKey(const std::string& filepath)
	{
		std::error_code error;
		return std::filesystem::weakly_canonical(std::filesystem::u8path(filepath), error).generic_u8string();
	}

	std::shared_ptr<AnimatorController> Get(const std::string& filepath)
	{
		if (filepath.empty())
		{
			return nullptr;
		}
		const std::string key = GetKey(filepath);
		auto it = s_Controllers.find(key);
		if (it != s_Controllers.end())
		{
			return it->second;
		}
		auto controller = std::make_shared<AnimatorController>();
		std::string error;
		if (!AnimatorController::Load(filepath, *controller, error))
		{
			Log::GetLogger()->error("Animator '{0}' could not be loaded: {1}", filepath, error);
			return nullptr; // tried again next time: the file may be written by then
		}
		s_Controllers[key] = controller;
		return controller;
	}

	bool Reload(const std::string& filepath, std::string& error)
	{
		auto it = s_Controllers.find(GetKey(filepath));
		if (it == s_Controllers.end())
		{
			return Get(filepath) != nullptr;
		}
		return AnimatorController::Load(filepath, *it->second, error);
	}

	std::vector<std::pair<std::string, std::shared_ptr<AnimatorController>>> GetLoaded()
	{
		return std::vector<std::pair<std::string, std::shared_ptr<AnimatorController>>>(s_Controllers.begin(), s_Controllers.end());
	}
}
