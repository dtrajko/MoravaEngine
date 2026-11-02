#include "AudioEngine.h"

#include "Core/Log.h"

#include <miniaudio.h>

#if defined(_WIN32)
	#include <objbase.h>
#endif

#include <cstring>
#include <filesystem>
#include <memory>


static const char* s_BusNames[(int)AudioBus::Count] = { "Music", "Effects", "UI" };

const char* GetAudioBusName(AudioBus bus)
{
	return (int)bus >= 0 && bus < AudioBus::Count ? s_BusNames[(int)bus] : "?";
}

bool FindAudioBus(const char* name, AudioBus& bus)
{
	for (int i = 0; i < (int)AudioBus::Count; i++)
	{
		if (std::strcmp(name, s_BusNames[i]) == 0)
		{
			bus = (AudioBus)i;
			return true;
		}
	}
	return false;
}

namespace AudioEngine
{
	// The engine and what it keeps pointers to: the context (its device's backend) and the buses
	struct State
	{
		ma_context Context;
		ma_engine Engine;
		ma_sound_group Buses[(int)AudioBus::Count];
		ma_sound Preview;
		bool HasPreview = false;
		bool Silent = false;
		bool Paused = false;
		AudioVolumes Volumes;
		std::string Description;
	};
	static std::unique_ptr<State> s_State;

	// The engine on the context's default output device. False (cleaned up) when there is none
	static bool InitEngine(State& state, const ma_backend* backends, ma_uint32 backendCount)
	{
		if (ma_context_init(backends, backendCount, nullptr, &state.Context) != MA_SUCCESS)
		{
			return false;
		}
		ma_engine_config config = ma_engine_config_init();
		config.pContext = &state.Context;
		if (ma_engine_init(&config, &state.Engine) != MA_SUCCESS)
		{
			ma_context_uninit(&state.Context);
			return false;
		}
		return true;
	}

	void Initialize()
	{
		if (s_State)
		{
			return;
		}
#if defined(_WIN32)
		// This is the main (UI) thread: COM single-threaded on it, as Windows' file dialogs need. miniaudio initializes COM
		// as multithreaded on the thread that starts it, and keeps it so: a file dialog opened later (New Project, Open
		// Scene) then hung when it browsed. With COM already set, miniaudio leaves it (RPC_E_CHANGED_MODE, which it expects);
		// its audio thread is its own, multithreaded
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif
		auto state = std::make_unique<State>();
		if (!InitEngine(*state, nullptr, 0))
		{
			// No audio device (or none that works): the null device consumes the sound at the same pace, silently
			const ma_backend null = ma_backend_null;
			if (!InitEngine(*state, &null, 1))
			{
				Log::GetLogger()->error("Audio: miniaudio could not start, not even without a device: no sound");
				return;
			}
			state->Silent = true;
		}
		for (ma_sound_group& bus : state->Buses)
		{
			ma_sound_group_init(&state->Engine, 0, nullptr, &bus);
		}

		ma_device* device = ma_engine_get_device(&state->Engine);
		state->Description = std::string(ma_get_backend_name(state->Context.backend)) + ": " +
			(state->Silent ? std::string("no audio device, sounds are silent") : std::string(device->playback.name)) + ", " +
			std::to_string(ma_engine_get_sample_rate(&state->Engine)) + " Hz, " + std::to_string(ma_engine_get_channels(&state->Engine)) +
			" channels";
		Log::GetLogger()->info("Audio: miniaudio {0}, {1}", ma_version_string(), state->Description);
		s_State = std::move(state);
	}

	void Shutdown()
	{
		if (!s_State)
		{
			return;
		}
		StopPreview();
		for (ma_sound_group& bus : s_State->Buses)
		{
			ma_sound_group_uninit(&bus);
		}
		ma_engine_uninit(&s_State->Engine);
		ma_context_uninit(&s_State->Context);
		s_State.reset();
	}

	bool IsAvailable() { return s_State != nullptr; }
	bool IsSilent() { return s_State && s_State->Silent; }

	const std::string& GetDeviceDescription()
	{
		static const std::string none = "not available";
		return s_State ? s_State->Description : none;
	}

	void SetVolumes(const AudioVolumes& volumes)
	{
		if (!s_State)
		{
			return;
		}
		s_State->Volumes = volumes;
		ma_engine_set_volume(&s_State->Engine, volumes.Master);
		for (int i = 0; i < (int)AudioBus::Count; i++)
		{
			ma_sound_group_set_volume(&s_State->Buses[i], volumes.Buses[i]);
		}
	}

	const AudioVolumes& GetVolumes()
	{
		static const AudioVolumes none;
		return s_State ? s_State->Volumes : none;
	}

	void SetPaused(bool paused)
	{
		if (!s_State || s_State->Paused == paused)
		{
			return;
		}
		s_State->Paused = paused;
		if (paused)
		{
			ma_engine_stop(&s_State->Engine);
		}
		else
		{
			ma_engine_start(&s_State->Engine);
		}
	}

	bool IsPaused() { return s_State && s_State->Paused; }

	ma_engine* GetEngine() { return s_State ? &s_State->Engine : nullptr; }

	ma_sound* GetBus(AudioBus bus)
	{
		return s_State && (int)bus >= 0 && bus < AudioBus::Count ? &s_State->Buses[(int)bus] : nullptr;
	}

	bool InitSound(const std::string& file, uint32_t flags, AudioBus bus, ma_sound* sound, std::string& error)
	{
		if (!s_State)
		{
			error = "no audio";
			return false;
		}
		const std::filesystem::path path = std::filesystem::u8path(file);
#if defined(_WIN32)
		// The wide path: miniaudio's narrow one is the system's code page on Windows, not UTF-8
		const ma_result result = ma_sound_init_from_file_w(&s_State->Engine, path.wstring().c_str(), flags, GetBus(bus), nullptr, sound);
#else
		const ma_result result = ma_sound_init_from_file(&s_State->Engine, path.c_str(), flags, GetBus(bus), nullptr, sound);
#endif
		if (result != MA_SUCCESS)
		{
			error = "'" + path.filename().u8string() + "' can't be played (" + ma_result_description(result) + ")";
			return false;
		}
		return true;
	}

	bool PlayPreview(const std::string& file, AudioBus bus, std::string& error)
	{
		StopPreview();
		if (!s_State)
		{
			error = "no audio";
			return false;
		}
		// Streamed: a long piece of music starts at once
		if (!InitSound(file, MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, bus, &s_State->Preview, error))
		{
			return false;
		}
		s_State->HasPreview = true;
		ma_sound_start(&s_State->Preview);
		return true;
	}

	void StopPreview()
	{
		if (s_State && s_State->HasPreview)
		{
			ma_sound_uninit(&s_State->Preview);
			s_State->HasPreview = false;
		}
	}

	bool IsPreviewPlaying()
	{
		return s_State && s_State->HasPreview && ma_sound_is_playing(&s_State->Preview);
	}
}
