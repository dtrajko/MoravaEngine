#pragma once

#include "AudioBus.h"

#include "EnvMapVulkan/EnvMapVulkanComponents.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


class EnvMapVulkanScene;
struct ma_sound;

/**
 * A running game's sounds (docs/engine/Game_Engine_Runtime_Guide.html, 5.1), in AudioEngine's engine: a sound for each
 * AudioSourceComponent, made when the game starts or the entity is placed (the ones with PlayOnStart start playing), and the one-shots the game
 * plays (Audio.PlayOneShot). GameSystems owns it, the same in the player and in the editor's Play.
 *
 * Each frame (Update): the listener (the entity with an active AudioListenerComponent, else the primary game camera) and the
 * spatial sounds follow their entities, with their velocities for the Doppler effect; finished one-shots and the sounds of
 * destroyed entities go. Stop: every sound stops and is freed. A clip's file is loaded (decoded) once per game, and shared
 * by every sound of it; Stream sounds read their file as they play.
 */
class SceneAudio
{
public:
	SceneAudio();
	~SceneAudio();
	SceneAudio(const SceneAudio&) = delete;
	SceneAudio& operator=(const SceneAudio&) = delete;

	void Start(EnvMapVulkanScene& scene);
	// Entities new in the running game (a prefab placed, 6.2): their sounds, the Play on Start ones playing
	void AddEntities(const std::vector<EnvMapVulkanEntityID>& entities);
	void Update(float frameTime);
	void Stop();
	bool IsRunning() const { return m_Scene != nullptr; }

	// The entity's sound (its AudioSourceComponent): false when it has none, or its clip couldn't be loaded. Play starts
	// it from the beginning, or goes on where Pause left it
	bool Play(EnvMapVulkanEntityID entity);
	bool Pause(EnvMapVulkanEntityID entity);
	bool StopSound(EnvMapVulkanEntityID entity); // and back to its beginning
	bool IsPlaying(EnvMapVulkanEntityID entity) const;
	// The component's Volume, Pitch and Loop changed (by a script, or in the Inspector during Play): the sound takes them
	bool ApplySettings(EnvMapVulkanEntityID entity);

	// A sound played once and forgotten: at a place in the world (position) or everywhere (nullptr). False (logged once
	// per clip) when the clip can't be loaded. At most MaxOneShots at once: the oldest stops for a new one. At a place:
	// as loud as volume up to minDistance meters from the listener, quieter farther (1 / distance)
	static constexpr size_t MaxOneShots = 64;
	bool PlayOneShot(const std::string& clip, const glm::vec3* position, AudioBus bus, float volume, float pitch, float minDistance = 1.0f);

	struct Stats
	{
		uint32_t Sources = 0;       // entities with a sound
		uint32_t Playing = 0;       // of them, playing now
		uint32_t OneShots = 0;      // playing now
		uint32_t Clips = 0;         // files loaded
		EnvMapVulkanEntityID Listener = NoEntity;
	};
	Stats GetStats() const;

private:
	struct EntitySound;
	struct OneShot;

	// The clip's sound loaded once (decoded): others are copies of it (nullptr: it couldn't be loaded, logged)
	ma_sound* GetClip(const std::string& clip);
	bool CreateSound(EntitySound& sound, const AudioSourceComponent& source);
	void UpdateListener(float frameTime);

	EnvMapVulkanScene* m_Scene = nullptr;
	std::map<EnvMapVulkanEntityID, std::unique_ptr<EntitySound>> m_Sounds;
	std::vector<std::unique_ptr<OneShot>> m_OneShots;
	std::map<std::string, std::unique_ptr<ma_sound>> m_Clips; // by file: never played, the copies share its data
	std::vector<std::string> m_ReportedClips; // logged once
	EnvMapVulkanEntityID m_Listener = NoEntity;
	glm::vec3 m_ListenerPosition = glm::vec3(0.0f);
	bool m_ListenerPlaced = false;
};
