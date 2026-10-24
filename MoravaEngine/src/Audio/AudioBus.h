#pragma once

#include <cstdint>


// The groups a game's sounds are mixed in (docs/engine/Game_Engine_Runtime_Guide.html, 5.1): each has its volume, and all
// of them go through the master volume. The numbers are saved in scenes by name and are C#'s AudioBus too
enum class AudioBus : int32_t { Music = 0, Effects = 1, UI = 2, Count };

const char* GetAudioBusName(AudioBus bus);
bool FindAudioBus(const char* name, AudioBus& bus); // false: no bus of that name

// The volumes a project mixes its sounds with (.mproject Audio: {Master: 1, Music: 0.8, Effects: 1, UI: 1}), 0 to 1
struct AudioVolumes
{
	float Master = 1.0f;
	float Buses[(int)AudioBus::Count] = { 1.0f, 1.0f, 1.0f };
};
