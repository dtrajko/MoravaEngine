#include "PlayerSettings.h"

#include "Core/Log.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>


bool PlayerSettings::IsEmpty() const
{
	bool volumes = MasterVolume.has_value();
	for (const std::optional<float>& bus : BusVolumes)
	{
		volumes |= bus.has_value();
	}
	return !Fullscreen && !Resolution && !VSync && !FrameRateLimit && !BatteryFrameRateLimit && !Quality && !volumes && Bindings.empty();
}

PlayerSettings PlayerSettings::Read(const YAML::Node& node)
{
	PlayerSettings settings;
	if (!node || !node.IsMap())
	{
		return settings;
	}
	// Each value on its own: one that can't be read doesn't take the others with it
	auto read = [&](const char* key, auto&& apply)
	{
		if (!node[key])
		{
			return;
		}
		try
		{
			apply(node[key]);
		}
		catch (const std::exception& e)
		{
			Log::GetLogger()->warn("Settings: the player's {0} is left out ({1})", key, e.what());
		}
	};
	read("Fullscreen", [&](const YAML::Node& value) { settings.Fullscreen = value.as<bool>(); });
	read("VSync", [&](const YAML::Node& value) { settings.VSync = value.as<bool>(); });
	read("FrameRateLimit", [&](const YAML::Node& value) { settings.FrameRateLimit = std::max(value.as<int>(), 0); });
	read("BatteryFrameRateLimit", [&](const YAML::Node& value) { settings.BatteryFrameRateLimit = std::max(value.as<int>(), 0); });
	read("Resolution", [&](const YAML::Node& value)
	{
		if (value.IsSequence() && value.size() == 2 && value[0].as<int>() > 0 && value[1].as<int>() > 0)
		{
			settings.Resolution = PlayerResolution{ value[0].as<uint32_t>(), value[1].as<uint32_t>() };
		}
	});
	read("Quality", [&](const YAML::Node& value)
	{
		QualityTier tier = QualityTier::Auto;
		if (FindQualityTier(value.as<std::string>(), tier) && tier != QualityTier::Auto)
		{
			settings.Quality = tier;
		}
	});
	read("Volumes", [&](const YAML::Node& volumes)
	{
		if (volumes["Master"])
		{
			settings.MasterVolume = std::max(volumes["Master"].as<float>(), 0.0f);
		}
		for (int i = 0; i < (int)AudioBus::Count; i++)
		{
			if (const YAML::Node bus = volumes[GetAudioBusName((AudioBus)i)])
			{
				settings.BusVolumes[i] = std::max(bus.as<float>(), 0.0f);
			}
		}
	});
	read("Bindings", [&](const YAML::Node& actions)
	{
		for (const auto& entry : actions)
		{
			const std::string action = entry.first.as<std::string>();
			std::vector<InputBinding> bindings;
			for (const YAML::Node& bindingNode : entry.second)
			{
				InputBinding binding;
				std::string error;
				if (InputMap::ReadBinding(bindingNode, binding, error))
				{
					bindings.push_back(std::move(binding));
				}
				else
				{
					Log::GetLogger()->warn("Settings: a binding the player gave '{0}' is left out ({1})", action, error);
				}
			}
			settings.Bindings[action] = std::move(bindings);
		}
	});
	return settings;
}

void PlayerSettings::Write(YAML::Emitter& out) const
{
	out << YAML::BeginMap;
	if (Fullscreen)
	{
		out << YAML::Key << "Fullscreen" << YAML::Value << *Fullscreen;
	}
	if (Resolution)
	{
		out << YAML::Key << "Resolution" << YAML::Value << YAML::Flow << YAML::BeginSeq << Resolution->Width << Resolution->Height << YAML::EndSeq;
	}
	if (VSync)
	{
		out << YAML::Key << "VSync" << YAML::Value << *VSync;
	}
	if (FrameRateLimit)
	{
		out << YAML::Key << "FrameRateLimit" << YAML::Value << *FrameRateLimit;
	}
	if (BatteryFrameRateLimit)
	{
		out << YAML::Key << "BatteryFrameRateLimit" << YAML::Value << *BatteryFrameRateLimit;
	}
	if (Quality)
	{
		out << YAML::Key << "Quality" << YAML::Value << GetQualityTierName(*Quality);
	}
	bool volumes = MasterVolume.has_value();
	for (const std::optional<float>& bus : BusVolumes)
	{
		volumes |= bus.has_value();
	}
	if (volumes)
	{
		out << YAML::Key << "Volumes" << YAML::Value << YAML::Flow << YAML::BeginMap;
		if (MasterVolume)
		{
			out << YAML::Key << "Master" << YAML::Value << *MasterVolume;
		}
		for (int i = 0; i < (int)AudioBus::Count; i++)
		{
			if (BusVolumes[i])
			{
				out << YAML::Key << GetAudioBusName((AudioBus)i) << YAML::Value << *BusVolumes[i];
			}
		}
		out << YAML::EndMap;
	}
	if (!Bindings.empty())
	{
		out << YAML::Key << "Bindings" << YAML::Value << YAML::BeginMap;
		for (const auto& [action, bindings] : Bindings)
		{
			out << YAML::Key << action << YAML::Value << YAML::Flow << YAML::BeginSeq;
			for (const InputBinding& binding : bindings)
			{
				InputMap::WriteBinding(out, binding);
			}
			out << YAML::EndSeq;
		}
		out << YAML::EndMap;
	}
	out << YAML::EndMap;
}

void PlayerSettings::ApplyBindings(InputMap& map) const
{
	for (const auto& [name, bindings] : Bindings)
	{
		for (InputAction& action : map.Actions)
		{
			if (action.Name == name)
			{
				action.Bindings = bindings;
			}
		}
	}
}

AudioVolumes PlayerSettings::ApplyVolumes(const AudioVolumes& project) const
{
	AudioVolumes volumes = project;
	if (MasterVolume)
	{
		volumes.Master = *MasterVolume;
	}
	for (int i = 0; i < (int)AudioBus::Count; i++)
	{
		if (BusVolumes[i])
		{
			volumes.Buses[i] = *BusVolumes[i];
		}
	}
	return volumes;
}

void PlayerSettings::SetVolumes(const AudioVolumes& volumes)
{
	MasterVolume = volumes.Master;
	for (int i = 0; i < (int)AudioBus::Count; i++)
	{
		BusVolumes[i] = volumes.Buses[i];
	}
}
