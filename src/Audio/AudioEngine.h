#pragma once

#include "AudioBus.h"

#include <cstdint>
#include <string>


struct ma_engine;
struct ma_sound;

/**
 * The process's audio (docs/engine/Game_Engine_Runtime_Guide.html, 5.1): miniaudio's engine on the system's default
 * output device, with a sound group per bus (AudioBus) under the master volume. It knows nothing of scenes: a running
 * game's sounds are SceneAudio's, made in this engine. Without an audio device (a server, CI) it plays into miniaudio's
 * null device: sounds are silent, and everything else works the same.
 *
 * Positions are in meters in the world; the listener and the 3D sounds use the engine's axes (Y up).
 */
namespace AudioEngine
{
	// Once per process (Application::OnInit), and at exit
	void Initialize();
	void Shutdown();

	bool IsAvailable();     // Initialize worked (also with the null device)
	bool IsSilent();        // the null device: no audio device was found
	const std::string& GetDeviceDescription(); // "WASAPI: Speakers (Realtek), 48000 Hz, 2 channels", for the log and the Audio panel

	// The volumes: the master and each bus's, 0 to 1 (above 1 amplifies)
	void SetVolumes(const AudioVolumes& volumes);
	const AudioVolumes& GetVolumes();

	// Every sound stops where it is (the editor's Pause) and goes on from there
	void SetPaused(bool paused);
	bool IsPaused();

	// The editor's preview of a sound file, not spatial, on its bus: one at a time (a new one replaces it)
	bool PlayPreview(const std::string& file, AudioBus bus, std::string& error);
	void StopPreview();
	bool IsPreviewPlaying();

	// For SceneAudio: miniaudio's engine and a bus's sound group (nullptr when not available)
	ma_engine* GetEngine();
	ma_sound* GetBus(AudioBus bus);
	// A sound of a file (a UTF-8 path) on a bus: ma_sound_init_from_file with miniaudio's MA_SOUND_FLAG_* flags. False
	// (error says why: the file's name and miniaudio's reason) when it can't be opened; the sound is then not initialized
	bool InitSound(const std::string& file, uint32_t flags, AudioBus bus, ma_sound* sound, std::string& error);
}
