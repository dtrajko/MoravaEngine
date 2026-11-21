#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <deque>
#include <string>
#include <vector>


/**
 * A decal's settings (docs/games/Shooter_Plan.html, E6; docs/engine/Game_Engine_Runtime_Guide.html, 12.4): plain data without
 * a graphics API, saved in the scene (DecalComponent) or given to a spawned one (Decals.Spawn). A decal is a box (the entity's
 * transform: a unit cube from -0.5 to 0.5, scaled; or a spawned one's), and its texture is projected along the box's -Y onto
 * the opaque surfaces inside it: bullet holes, scorch marks, stains, posters.
 *
 * It darkens what is under it, as the trails do: the surface's lit color times the texture's color times Color, by the
 * texture's alpha times Color's alpha (the light the surface gets stays on the mark). Surfaces turned away from the box's up
 * fade out (no long smears down a wall's side), and so do the box's ends along its up. Characters and the first person's arms
 * aren't spared: a decal is drawn on whatever is inside its box (keep spawned ones thin).
 */
struct DecalSettings
{
	std::string Texture;             // absolute here; in the scene file an asset reference. Empty: a soft round spot
	glm::vec4 Color = glm::vec4(1.0f); // times the texture; alpha: how much it covers
};

// A short light (a muzzle flash, an explosion): a point light that fades out over its seconds (Lights.Flash)
struct FlashLight
{
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Color = glm::vec3(1.0f);
	float Intensity = 1.0f; // at its start; it falls with the square of the time left
	float Range = 5.0f;
	float Seconds = 0.05f;
	float Age = 0.0f;

	float CurrentIntensity() const;
};

// A decal a script spawned (Decals.Spawn): not an entity, kept in the scene's pool
struct SpawnedDecal
{
	glm::mat4 Transform = glm::mat4(1.0f); // the box (a unit cube, scaled and placed)
	DecalSettings Settings;
	float Lifetime = 0.0f;                 // seconds (0: until the pool needs its place)
	float Age = 0.0f;
	float FadeFrom = -1.0f;                // the age its fade out began (-1: not fading); it fades over FadeSeconds

	float Opacity() const;                 // 1, less while it fades out
};

/**
 * What a running game adds to its scene for a short while (lights and decals that aren't entities): kept with the scene,
 * not saved, and a copy of the scene (Play) starts without them. Aged by the game's time (GameSystems::Update); the renderer
 * reads them each frame.
 */
struct SceneEffects
{
	static constexpr size_t MaxFlashes = 8;   // at once (the renderer has 16 point lights in all; the scene's go first)
	static constexpr size_t MaxDecals = 512;  // spawned ones at once: past this the oldest fade out
	static constexpr float FadeSeconds = 0.5f;

	std::vector<FlashLight> Flashes;
	std::deque<SpawnedDecal> Decals;

	// A flash: when all MaxFlashes are in use, the one nearest its end makes room
	void AddFlash(const FlashLight& flash);
	// A decal: the oldest of those not fading starts to fade when there are MaxDecals already (and one past MaxDecals plus a
	// fade's worth goes at once)
	void SpawnDecal(const SpawnedDecal& decal);
	// The game's time went on: flashes and decals age, the ones past their end go
	void Update(float deltaTime);
	void Clear();
};
