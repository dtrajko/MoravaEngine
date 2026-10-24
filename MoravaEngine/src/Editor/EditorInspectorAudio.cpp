// The Inspector's Audio section (docs/engine/Game_Engine_Runtime_Guide.html, 5.3): the entity's Audio Source (its clip,
// bus and settings, with a preview of the clip) and Audio Listener. While the game runs, the settings are the running
// sound's: a changed volume, pitch, loop or distance is heard at once, a new clip, bus or Spatial at its next Play (the
// Play button here plays it), and every change lasts until Stop.


#include "Audio/AudioEngine.h"
#include "Audio/SceneAudio.h"

#include "Project/Project.h"

#include "Core/FileDialogs.h"
#include "Core/Util.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>


#include "EditorState.h"


static const char* s_AudioExtensions = "wav,flac,mp3,ogg";

static bool IsAudioFile(const std::filesystem::path& path)
{
	std::string extension = path.extension().u8string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return extension == ".wav" || extension == ".flac" || extension == ".mp3" || extension == ".ogg";
}

// The clip: its file (relative to the project), chosen with a file dialog or dropped from the Content Browser
static bool ClipControls(AudioSourceComponent& source)
{
	bool changed = false;
	const std::string shown = source.Clip.empty() ? std::string("(none: choose a sound file)") : ProjectPaths::ToStored(source.Clip);
	ImGui::SetNextItemWidth(-90.0f);
	ImGui::BeginDisabled(true);
	char buffer[512] = {};
	snprintf(buffer, sizeof(buffer), "%s", shown.c_str());
	ImGui::InputText("##Clip", buffer, sizeof(buffer), ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
		{
			std::error_code error;
			const std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(Util::to_str((const wchar_t*)payload->Data)), error);
			if (IsAudioFile(path))
			{
				source.Clip = path.lexically_normal().generic_u8string();
				changed = true;
			}
			else
			{
				ShowNotification("A sound is a WAV, FLAC, MP3 or Ogg Vorbis file", true);
			}
		}
		ImGui::EndDragDropTarget();
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("%s\nWAV, FLAC, MP3 or Ogg Vorbis. Drop a sound file from the Content Browser here, or choose one with ...",
			source.Clip.empty() ? "No clip yet" : source.Clip.c_str());
	}
	ImGui::SameLine();
	if (ImGui::Button("...##ChooseClip"))
	{
		const std::string file = FileDialogs::OpenFile({ { "Sound", s_AudioExtensions } }, GetAssetFolderForDialog());
		if (!file.empty())
		{
			source.Clip = std::filesystem::u8path(file).lexically_normal().generic_u8string();
			changed = true;
		}
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Clip");
	return changed;
}

// Plays the clip as the editor's preview (not spatial), or the running game's sound
static void PlayControls(EnvMapVulkanEntityID entity, const AudioSourceComponent& source)
{
	SceneAudio* game = GetGameAudio();
	if (game)
	{
		const bool playing = game->IsPlaying(entity);
		if (ImGui::SmallButton(playing ? "Stop##GameSound" : "Play##GameSound"))
		{
			if (playing)
			{
				game->StopSound(entity);
			}
			else if (!game->Play(entity))
			{
				ShowNotification("The sound can't be played: the log says why", true);
			}
		}
		ImGui::SameLine();
		ImGui::TextDisabled(playing ? "Playing" : "Not playing");
		return;
	}
	ImGui::BeginDisabled(source.Clip.empty() || !AudioEngine::IsAvailable() || IsPlaying());
	const bool previewing = AudioEngine::IsPreviewPlaying();
	if (ImGui::SmallButton(previewing ? "Stop Preview" : "Preview"))
	{
		if (previewing)
		{
			AudioEngine::StopPreview();
		}
		else
		{
			// At the project's volumes (the mixer's): as the game will play it
			const Project* project = Project::GetOpen();
			AudioEngine::SetVolumes(project ? project->Volumes : AudioVolumes());
			std::string error;
			if (!AudioEngine::PlayPreview(source.Clip, source.Bus, error))
			{
				ShowNotification(error, true);
			}
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(AudioEngine::IsSilent() ? "No audio device: sounds are silent" :
			"The clip once, on its bus, the same everywhere (in the game it is heard from the entity when Spatial)");
	}
}

void AudioControls(EnvMapVulkanEntityID entity)
{
	ImGui::Separator();
	ImGui::PushID("Audio");

	bool hasSource = s_Scene.Has<AudioSourceComponent>(entity);
	// A Sound entity is its source: removing it is deleting the entity (Delete)
	ImGui::BeginDisabled(GetEntityKind(entity) == EntityKind::Sound);
	if (ImGui::Checkbox("Audio Source", &hasSource))
	{
		if (hasSource)
		{
			s_Scene.Add<AudioSourceComponent>(entity);
		}
		else
		{
			s_Scene.Remove<AudioSourceComponent>(entity);
		}
	}
	ImGui::EndDisabled();
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("A sound the entity plays while the game runs, from where it is (Spatial) or the same everywhere\n"
			"(music). Scripts play it with GetComponent<AudioSource>().Play().");
	}
	ImGui::SameLine();
	bool hasListener = s_Scene.Has<AudioListenerComponent>(entity);
	if (ImGui::Checkbox("Audio Listener", &hasListener))
	{
		if (hasListener)
		{
			s_Scene.Add<AudioListenerComponent>(entity);
		}
		else
		{
			s_Scene.Remove<AudioListenerComponent>(entity);
		}
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The game is heard from here: from its position, facing its forward (-Z; a camera: where it looks).\n"
			"Usually on the camera. Without one, the scene's camera listens.");
	}
	if (AudioListenerComponent* listener = s_Scene.TryGet<AudioListenerComponent>(entity))
	{
		ImGui::SameLine();
		ImGui::Checkbox("Active##Listener", &listener->Active);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("The first active listener in the scene listens");
		}
	}

	AudioSourceComponent* source = s_Scene.TryGet<AudioSourceComponent>(entity);
	if (!source)
	{
		ImGui::PopID();
		return;
	}

	ClipControls(*source);
	PlayControls(entity, *source);

	bool settingsChanged = false;
	int bus = (int)source->Bus;
	if (ImGui::BeginCombo("Bus", GetAudioBusName(source->Bus)))
	{
		for (int i = 0; i < (int)AudioBus::Count; i++)
		{
			if (ImGui::Selectable(GetAudioBusName((AudioBus)i), i == bus))
			{
				source->Bus = (AudioBus)i;
			}
		}
		ImGui::EndCombo();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The group it is mixed in: each has its volume (View > Audio). A new bus from the sound's next Play.");
	}
	settingsChanged |= ImGui::SliderFloat("Volume", &source->Volume, 0.0f, 1.0f, "%.2f");
	settingsChanged |= ImGui::SliderFloat("Pitch", &source->Pitch, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	source->Pitch = std::max(source->Pitch, 0.01f);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("1: as recorded. 2: an octave higher and twice as fast; 0.5: an octave lower and half as fast.");
	}
	settingsChanged |= ImGui::Checkbox("Loop", &source->Loop);
	ImGui::SameLine();
	ImGui::Checkbox("Play on Start", &source->PlayOnStart);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("It plays when the game starts. Else a script plays it: GetComponent<AudioSource>().Play()");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Stream", &source->Stream);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Read from its file while it plays, not loaded first: for music and other long sounds.\n"
			"Short sounds load once and are shared by every entity that plays them.");
	}
	ImGui::Checkbox("Spatial", &source->Spatial);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Heard from where the entity is: louder near the listener, from its side.\n"
			"Off: the same everywhere (music, a menu's sounds).");
	}
	if (source->Spatial)
	{
		settingsChanged |= ImGui::DragFloatRange2("Distance", &source->MinDistance, &source->MaxDistance, 0.1f, 0.01f, 1000.0f,
			"Min %.1f m", "Max %.1f m", ImGuiSliderFlags_AlwaysClamp);
		source->MaxDistance = std::max(source->MaxDistance, source->MinDistance);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Full volume up to Min, then softer with distance (inverse), no softer beyond Max.\n"
				"The viewport shows both as spheres around the selected source.");
		}
	}
	if (settingsChanged)
	{
		if (SceneAudio* game = GetGameAudio())
		{
			game->ApplySettings(entity);
		}
	}

	ImGui::PopID();
}
