#pragma once

#include "Audio/AudioBus.h"

#include "Input/InputMap.h"

#include "Renderer/QualityTier.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>


namespace YAML { class Emitter; class Node; }

// A fullscreen resolution: one of the monitor's video modes
struct PlayerResolution
{
	uint32_t Width = 0, Height = 0;

	bool operator==(const PlayerResolution& other) const { return Width == other.Width && Height == other.Height; }
	bool operator!=(const PlayerResolution& other) const { return !(*this == other); }
};

/**
 * What the player chose in the game's settings menu (docs/engine/Game_Engine_Runtime_Guide.html, 11.4): the window, the
 * quality tier, the volumes and the bindings. The engine keeps them, in the Player section of the game's settings file
 * (GameSettings), and puts them in place when the game starts: the window before it opens, the rest before the first
 * scene. A value never chosen is the project's (Auto, for the quality: the GPU's tier).
 *
 * C# sets them where they belong: Application.Fullscreen, Application.Resolution, Application.VSync,
 * Application.FrameRateLimit, Application.BatteryFrameRateLimit, Quality.Level,
 * Audio.SetBusVolume, Input.Rebind.
 *
 *   Player:
 *     Fullscreen: true
 *     Resolution: [1920, 1080]
 *     VSync: true
 *     FrameRateLimit: 144
 *     BatteryFrameRateLimit: 60
 *     Quality: Medium
 *     Volumes: {Master: 1, Music: 0.4}
 *     Bindings:
 *       Jump: [{Key: J}, {Gamepad: A}]   # all of an action's bindings, once one of them changed
 */
struct PlayerSettings
{
	static constexpr const char* Section = "Player";

	std::optional<bool> Fullscreen;
	std::optional<PlayerResolution> Resolution; // fullscreen: the video mode (none: the monitor's own)
	std::optional<bool> VSync;
	std::optional<int> FrameRateLimit;          // frames a second at most, 0: none (FrameLimiter)
	std::optional<int> BatteryFrameRateLimit;   // on battery, 0: as plugged in
	std::optional<QualityTier> Quality;         // never Auto: no value is Auto
	std::optional<float> MasterVolume;
	std::optional<float> BusVolumes[(int)AudioBus::Count];
	std::map<std::string, std::vector<InputBinding>> Bindings; // the actions the player rebound

	bool IsEmpty() const;

	// From the Player section (a value that can't be read is left out, with a warning), and into it
	static PlayerSettings Read(const YAML::Node& node);
	void Write(YAML::Emitter& out) const;

	// The project's map with the player's bindings in place (an action the map no longer has is left out)
	void ApplyBindings(InputMap& map) const;
	// The project's volumes with the player's in place
	AudioVolumes ApplyVolumes(const AudioVolumes& project) const;
	// Remembers the volumes as the player's
	void SetVolumes(const AudioVolumes& volumes);
};
