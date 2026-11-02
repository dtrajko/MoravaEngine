// The Audio panel (View > Audio, docs/engine/Game_Engine_Runtime_Guide.html, 5.3): the audio device, the mixer (the
// master and the buses' volumes, the project's: saved with it, and heard at once), the viewport's sound icons, and while
// the game runs its sounds: how many play, the clips loaded, the listener, and each entity's sound (click: select it).


#include "Audio/AudioEngine.h"
#include "Audio/SceneAudio.h"

#include "Project/Project.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>
#include <string>


#include "EditorState.h"


// The mixer: the project's volumes (no project: the engine's, for this session)
static void MixerControls()
{
	Project* project = Project::GetOpen();
	AudioVolumes volumes = project ? project->Volumes : AudioEngine::GetVolumes();
	bool changed = false, save = false;
	auto slider = [&](const char* label, float& volume, const char* tooltip)
	{
		changed |= ImGui::SliderFloat(label, &volume, 0.0f, 1.0f, "%.2f");
		save |= ImGui::IsItemDeactivatedAfterEdit();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
	};
	slider("Master", volumes.Master, "Everything goes through it");
	slider("Music", volumes.Buses[(int)AudioBus::Music], "The sounds on the Music bus (Inspector: Audio Source, Bus)");
	slider("Effects", volumes.Buses[(int)AudioBus::Effects], "The game's sounds: the Effects bus, and one-shots unless they name another");
	slider("UI", volumes.Buses[(int)AudioBus::UI], "Menus and buttons: the UI bus");
	if (changed)
	{
		AudioEngine::SetVolumes(volumes); // heard at once, also by a running game
		if (project)
		{
			project->Volumes = volumes;
		}
	}
	if (save && project)
	{
		SaveProject();
	}
	ImGui::TextDisabled(project ? "The project's volumes: saved with it (.mproject Audio), the game starts with them." :
		"No project: these volumes last for this session.");
	if (GetGameAudio())
	{
		ImGui::TextDisabled("A script's Audio.SetBusVolume lasts until Stop.");
	}
}

static void GameSoundsControls(SceneAudio& audio)
{
	const SceneAudio::Stats stats = audio.GetStats();
	ImGui::Text("%u sound(s) of entities, %u playing; %u one-shot(s) playing; %u clip(s) loaded", stats.Sources, stats.Playing,
		stats.OneShots, stats.Clips);
	const NameComponent* listenerName = s_Scene.TryGet<NameComponent>(stats.Listener);
	ImGui::Text("Listener: %s", listenerName ? listenerName->Name.c_str() : "none (no camera and no Audio Listener)");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The first entity with an active Audio Listener, else the primary game camera");
	}

	if (ImGui::BeginTable("##Sounds", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY,
		ImVec2(0.0f, std::max(120.0f, ImGui::GetContentRegionAvail().y))))
	{
		ImGui::TableSetupColumn("Entity");
		ImGui::TableSetupColumn("Clip");
		ImGui::TableSetupColumn("Bus", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableHeadersRow();
		s_Scene.Each<AudioSourceComponent>([&](EnvMapVulkanEntityID entity, AudioSourceComponent& source)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const bool playing = audio.IsPlaying(entity);
			const NameComponent* name = s_Scene.TryGet<NameComponent>(entity);
			char label[160];
			snprintf(label, sizeof(label), "%s %s##%llu", playing ? ">" : " ", name ? name->Name.c_str() : "?", (unsigned long long)entity);
			if (ImGui::Selectable(label, s_EditorSelection.IsSelected(entity), ImGuiSelectableFlags_SpanAllColumns))
			{
				s_EditorSelection.SelectOnly(entity);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(playing ? "Playing (click: select it)" : "Not playing (click: select it)");
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(ProjectPaths::ToStored(source.Clip).c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(GetAudioBusName(source.Bus));
		});
		ImGui::EndTable();
	}
}

void OnImGuiRenderAudioPanel()
{
	if (!s_ShowAudioPanel)
	{
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(440.0f, 480.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Audio", &s_ShowAudioPanel))
	{
		ImGui::End();
		return;
	}

	ImGui::TextWrapped("Device: %s", AudioEngine::GetDeviceDescription().c_str());
	if (AudioEngine::IsSilent())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "No audio device: the game runs, its sounds are silent.");
	}
	if (AudioEngine::IsPreviewPlaying())
	{
		if (ImGui::SmallButton("Stop Preview"))
		{
			AudioEngine::StopPreview();
		}
	}

	ImGui::SeparatorText("Mixer");
	MixerControls();

	ImGui::SeparatorText("Viewport");
	ImGui::Checkbox("Sound Icons and Distances", &s_ShowAudioGizmos);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("A speaker on every sound (bright while it plays; click it to select), and the selected\n"
			"spatial sound's Min and Max distances as spheres");
	}

	ImGui::SeparatorText("The Game's Sounds");
	if (SceneAudio* audio = GetGameAudio())
	{
		GameSoundsControls(*audio);
	}
	else
	{
		ImGui::TextWrapped(IsPlaying() ? "Simulate plays no sounds (physics only)." :
			"The game isn't running: its sounds are made at Play, from the entities' Audio Sources.");
	}

	ImGui::End();
}
