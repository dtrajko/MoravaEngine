#include "SceneAudio.h"

#include "AudioEngine.h"

#include "EnvMapVulkan/EnvMapVulkanScene.h"

#include "Core/Log.h"

#include <miniaudio.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>


// An entity's sound and where it was in the last frame (its velocity)
struct SceneAudio::EntitySound
{
	ma_sound Sound;
	bool Initialized = false;
	bool Spatial = true;
	glm::vec3 Position = glm::vec3(0.0f);
	bool Placed = false;
	std::string Clip; // the one it was made of: a changed clip makes a new sound
	bool Stream = false;
};

struct SceneAudio::OneShot
{
	ma_sound Sound;
};

// A position jump faster than this (meters per second) is a teleport, not a movement: no Doppler shift for it
static constexpr float MaxSoundSpeed = 100.0f;

static glm::vec3 GetVelocity(const glm::vec3& from, const glm::vec3& to, float frameTime)
{
	if (frameTime <= 0.0f)
	{
		return glm::vec3(0.0f);
	}
	const glm::vec3 velocity = (to - from) / frameTime;
	return glm::length(velocity) > MaxSoundSpeed ? glm::vec3(0.0f) : velocity;
}

SceneAudio::SceneAudio() = default;

SceneAudio::~SceneAudio()
{
	Stop();
}

ma_sound* SceneAudio::GetClip(const std::string& clip)
{
	if (auto it = m_Clips.find(clip); it != m_Clips.end())
	{
		return it->second.get();
	}
	auto sound = std::make_unique<ma_sound>();
	std::string error;
	if (!AudioEngine::InitSound(clip, MA_SOUND_FLAG_DECODE, AudioBus::Effects, sound.get(), error))
	{
		if (std::find(m_ReportedClips.begin(), m_ReportedClips.end(), clip) == m_ReportedClips.end())
		{
			m_ReportedClips.push_back(clip);
			Log::GetLogger()->error("Audio: {0}", error);
		}
		m_Clips[clip] = nullptr; // not tried again in this game
		return nullptr;
	}
	return (m_Clips[clip] = std::move(sound)).get();
}

bool SceneAudio::CreateSound(EntitySound& sound, const AudioSourceComponent& source)
{
	ma_engine* engine = AudioEngine::GetEngine();
	if (!engine || source.Clip.empty())
	{
		return false;
	}
	const ma_uint32 spatial = source.Spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;
	if (source.Stream)
	{
		std::string error;
		if (!AudioEngine::InitSound(source.Clip, MA_SOUND_FLAG_STREAM | spatial, source.Bus, &sound.Sound, error))
		{
			Log::GetLogger()->error("Audio: {0}", error);
			return false;
		}
	}
	else
	{
		ma_sound* clip = GetClip(source.Clip);
		if (!clip || ma_sound_init_copy(engine, clip, spatial, AudioEngine::GetBus(source.Bus), &sound.Sound) != MA_SUCCESS)
		{
			return false;
		}
	}
	sound.Initialized = true;
	sound.Spatial = source.Spatial;
	sound.Clip = source.Clip;
	sound.Stream = source.Stream;
	if (source.Spatial)
	{
		ma_sound_set_attenuation_model(&sound.Sound, ma_attenuation_model_inverse);
		ma_sound_set_min_distance(&sound.Sound, std::max(source.MinDistance, 0.01f));
		ma_sound_set_max_distance(&sound.Sound, std::max(source.MaxDistance, source.MinDistance));
	}
	ma_sound_set_volume(&sound.Sound, source.Volume);
	ma_sound_set_pitch(&sound.Sound, std::max(source.Pitch, 0.01f));
	ma_sound_set_looping(&sound.Sound, source.Loop ? MA_TRUE : MA_FALSE);
	return true;
}

void SceneAudio::Start(EnvMapVulkanScene& scene)
{
	Stop();
	if (!AudioEngine::IsAvailable())
	{
		return;
	}
	m_Scene = &scene;
	AudioEngine::StopPreview();
	scene.Each<AudioSourceComponent>([&](EnvMapVulkanEntityID entity, AudioSourceComponent& source)
	{
		auto sound = std::make_unique<EntitySound>();
		CreateSound(*sound, source);
		m_Sounds[entity] = std::move(sound);
	});
	// Placed before the first sound plays, so it doesn't start from the world's origin
	Update(0.0f);
	uint32_t started = 0;
	scene.Each<AudioSourceComponent>([&](EnvMapVulkanEntityID entity, AudioSourceComponent& source)
	{
		if (source.PlayOnStart && Play(entity))
		{
			started++;
		}
	});
	if (!m_Sounds.empty())
	{
		Log::GetLogger()->info("Audio: {0} sound(s), {1} playing from the start, {2} clip(s) loaded", m_Sounds.size(), started,
			m_Clips.size());
	}
}

void SceneAudio::Stop()
{
	if (!AudioEngine::IsAvailable())
	{
		// Shut down already (the process ends): the sounds went with the engine
		m_Sounds.clear();
		m_OneShots.clear();
		m_Clips.clear();
		m_Scene = nullptr;
		return;
	}
	for (auto& [entity, sound] : m_Sounds)
	{
		if (sound->Initialized)
		{
			ma_sound_uninit(&sound->Sound);
		}
	}
	m_Sounds.clear();
	for (auto& oneShot : m_OneShots)
	{
		ma_sound_uninit(&oneShot->Sound);
	}
	m_OneShots.clear();
	// The copies are gone: now their clips
	for (auto& [file, clip] : m_Clips)
	{
		if (clip)
		{
			ma_sound_uninit(clip.get());
		}
	}
	m_Clips.clear();
	m_ReportedClips.clear();
	m_Scene = nullptr;
	m_Listener = NoEntity;
	m_ListenerPlaced = false;
}

void SceneAudio::UpdateListener(float frameTime)
{
	ma_engine* engine = AudioEngine::GetEngine();

	// The active listener, else the camera
	m_Listener = NoEntity;
	m_Scene->Each<AudioListenerComponent>([&](EnvMapVulkanEntityID entity, AudioListenerComponent& listener)
	{
		if (m_Listener == NoEntity && listener.Active)
		{
			m_Listener = entity;
		}
	});
	if (m_Listener == NoEntity)
	{
		m_Listener = m_Scene->FindFirst<CameraComponent>();
	}
	if (m_Listener == NoEntity)
	{
		return;
	}

	const glm::mat4 world = m_Scene->GetWorldTransform(m_Listener);
	const glm::vec3 position = glm::vec3(world[3]);
	glm::vec3 forward = -glm::vec3(world[2]);
	glm::vec3 up = glm::vec3(world[1]);
	if (m_Scene->Has<CameraComponent>(m_Listener))
	{
		// The camera looks as the renderer places it (EnvMapVulkanSceneData::PlaceCamera): X down (positive) and up, Y
		// around the vertical axis, 0 along -Z; never rolled
		const glm::vec3& rotation = m_Scene->Get<TransformComponent>(m_Listener).Rotation;
		const float pitch = glm::radians(glm::clamp(rotation.x, -89.0f, 89.0f));
		const float yaw = glm::radians(-rotation.y - 90.0f);
		forward = glm::vec3(std::cos(yaw) * std::cos(pitch), -std::sin(pitch), std::sin(yaw) * std::cos(pitch));
		up = glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if (glm::length(forward) < 1e-6f || glm::length(up) < 1e-6f)
	{
		return;
	}
	const glm::vec3 velocity = m_ListenerPlaced ? GetVelocity(m_ListenerPosition, position, frameTime) : glm::vec3(0.0f);
	m_ListenerPosition = position;
	m_ListenerPlaced = true;
	forward = glm::normalize(forward);
	ma_engine_listener_set_position(engine, 0, position.x, position.y, position.z);
	ma_engine_listener_set_direction(engine, 0, forward.x, forward.y, forward.z);
	ma_engine_listener_set_world_up(engine, 0, up.x, up.y, up.z);
	ma_engine_listener_set_velocity(engine, 0, velocity.x, velocity.y, velocity.z);
}

void SceneAudio::Update(float frameTime)
{
	if (!m_Scene)
	{
		return;
	}
	UpdateListener(frameTime);

	for (auto it = m_Sounds.begin(); it != m_Sounds.end();)
	{
		EntitySound& sound = *it->second;
		// Gone with its entity (a script destroyed it, or the editor), or its sound
		if (!m_Scene->Exists(it->first) || !m_Scene->Has<AudioSourceComponent>(it->first))
		{
			if (sound.Initialized)
			{
				ma_sound_uninit(&sound.Sound);
			}
			it = m_Sounds.erase(it);
			continue;
		}
		if (sound.Initialized && sound.Spatial)
		{
			const glm::vec3 position = glm::vec3(m_Scene->GetWorldTransform(it->first)[3]);
			const glm::vec3 velocity = sound.Placed ? GetVelocity(sound.Position, position, frameTime) : glm::vec3(0.0f);
			sound.Position = position;
			sound.Placed = true;
			ma_sound_set_position(&sound.Sound, position.x, position.y, position.z);
			ma_sound_set_velocity(&sound.Sound, velocity.x, velocity.y, velocity.z);
		}
		++it;
	}

	// Finished one-shots
	m_OneShots.erase(std::remove_if(m_OneShots.begin(), m_OneShots.end(), [](const std::unique_ptr<OneShot>& oneShot)
	{
		if (ma_sound_at_end(&oneShot->Sound) || !ma_sound_is_playing(&oneShot->Sound))
		{
			ma_sound_uninit(&oneShot->Sound);
			return true;
		}
		return false;
	}), m_OneShots.end());
}

bool SceneAudio::Play(EnvMapVulkanEntityID entity)
{
	auto it = m_Sounds.find(entity);
	const AudioSourceComponent* source = m_Scene ? m_Scene->TryGet<AudioSourceComponent>(entity) : nullptr;
	if (!source)
	{
		return false;
	}
	if (it == m_Sounds.end())
	{
		// Added to the entity while the game runs
		it = m_Sounds.emplace(entity, std::make_unique<EntitySound>()).first;
	}
	EntitySound& sound = *it->second;
	if (sound.Initialized && (sound.Clip != source->Clip || sound.Stream != source->Stream || sound.Spatial != source->Spatial))
	{
		ma_sound_uninit(&sound.Sound);
		sound = EntitySound();
	}
	if (!sound.Initialized && !CreateSound(sound, *source))
	{
		return false;
	}
	ApplySettings(entity);
	if (ma_sound_at_end(&sound.Sound))
	{
		ma_sound_seek_to_pcm_frame(&sound.Sound, 0);
	}
	return ma_sound_start(&sound.Sound) == MA_SUCCESS;
}

bool SceneAudio::Pause(EnvMapVulkanEntityID entity)
{
	auto it = m_Sounds.find(entity);
	if (it == m_Sounds.end() || !it->second->Initialized)
	{
		return m_Scene && m_Scene->Has<AudioSourceComponent>(entity); // not playing: nothing to pause
	}
	return ma_sound_stop(&it->second->Sound) == MA_SUCCESS;
}

bool SceneAudio::StopSound(EnvMapVulkanEntityID entity)
{
	auto it = m_Sounds.find(entity);
	if (it == m_Sounds.end() || !it->second->Initialized)
	{
		return m_Scene && m_Scene->Has<AudioSourceComponent>(entity);
	}
	ma_sound_stop(&it->second->Sound);
	ma_sound_seek_to_pcm_frame(&it->second->Sound, 0);
	return true;
}

bool SceneAudio::IsPlaying(EnvMapVulkanEntityID entity) const
{
	auto it = m_Sounds.find(entity);
	return it != m_Sounds.end() && it->second->Initialized && ma_sound_is_playing(&it->second->Sound);
}

bool SceneAudio::ApplySettings(EnvMapVulkanEntityID entity)
{
	auto it = m_Sounds.find(entity);
	const AudioSourceComponent* source = m_Scene ? m_Scene->TryGet<AudioSourceComponent>(entity) : nullptr;
	if (!source)
	{
		return false;
	}
	if (it == m_Sounds.end() || !it->second->Initialized)
	{
		return true; // taken when it plays
	}
	ma_sound* sound = &it->second->Sound;
	ma_sound_set_volume(sound, source->Volume);
	ma_sound_set_pitch(sound, std::max(source->Pitch, 0.01f));
	ma_sound_set_looping(sound, source->Loop ? MA_TRUE : MA_FALSE);
	if (it->second->Spatial)
	{
		ma_sound_set_min_distance(sound, std::max(source->MinDistance, 0.01f));
		ma_sound_set_max_distance(sound, std::max(source->MaxDistance, source->MinDistance));
	}
	return true;
}

bool SceneAudio::PlayOneShot(const std::string& clip, const glm::vec3* position, AudioBus bus, float volume, float pitch)
{
	ma_engine* engine = AudioEngine::GetEngine();
	ma_sound* source = m_Scene && engine ? GetClip(clip) : nullptr;
	if (!source)
	{
		return false;
	}
	if (m_OneShots.size() >= MaxOneShots)
	{
		ma_sound_uninit(&m_OneShots.front()->Sound);
		m_OneShots.erase(m_OneShots.begin());
	}
	auto oneShot = std::make_unique<OneShot>();
	if (ma_sound_init_copy(engine, source, position ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION, AudioEngine::GetBus(bus), &oneShot->Sound)
		!= MA_SUCCESS)
	{
		return false;
	}
	if (position)
	{
		ma_sound_set_attenuation_model(&oneShot->Sound, ma_attenuation_model_inverse);
		ma_sound_set_position(&oneShot->Sound, position->x, position->y, position->z);
	}
	ma_sound_set_volume(&oneShot->Sound, volume);
	ma_sound_set_pitch(&oneShot->Sound, std::max(pitch, 0.01f));
	if (ma_sound_start(&oneShot->Sound) != MA_SUCCESS)
	{
		ma_sound_uninit(&oneShot->Sound);
		return false;
	}
	m_OneShots.push_back(std::move(oneShot));
	return true;
}

SceneAudio::Stats SceneAudio::GetStats() const
{
	Stats stats;
	stats.Sources = (uint32_t)m_Sounds.size();
	for (const auto& [entity, sound] : m_Sounds)
	{
		if (sound->Initialized && ma_sound_is_playing(&sound->Sound))
		{
			stats.Playing++;
		}
	}
	stats.OneShots = (uint32_t)m_OneShots.size();
	for (const auto& [file, clip] : m_Clips)
	{
		stats.Clips += clip ? 1 : 0;
	}
	stats.Listener = m_Listener;
	return stats;
}
