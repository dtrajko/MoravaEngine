#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>


/**
 * A particle emitter's settings (docs/engine/Game_Engine_Runtime_Guide.html, 12.1; the racing game's tire smoke): plain data
 * without a graphics API, saved in the scene (ParticleEmitterComponent). The renderer starts, moves and draws the particles
 * (EnvMapVulkanParticles): on the GPU, each emitter in a slice of one pool, MaxParticles long, where a new particle takes the
 * place of the oldest.
 *
 * A particle starts at the emitter (anywhere within StartRadius of it; along the way the emitter moved since the last frame,
 * so a fast emitter leaves an even trail), moving at a speed between SpeedMin and SpeedMax in a direction within ConeAngle
 * degrees of the entity's up (+Y), plus InheritVelocity times the emitter's own velocity. It lives LifetimeMin to LifetimeMax
 * seconds, pulled by Acceleration (world, m/s²: gravity is (0, -9.81, 0); rising smoke has a small upward one) and slowed by
 * Drag (per second: the share of its speed it loses). Its size and color (with alpha) go from Start to End over its life; it
 * turns at a random speed up to SpinSpeed degrees a second.
 *
 * Drawn as a square facing the camera: the Texture (its alpha the particle's shape), or a soft round puff without one.
 * Alpha: blended over the scene by its alpha; Additive: added to it (fire, sparks). Lit: shaded by the sun (with its shadow)
 * and the ambient light; else its color as it is. SoftDistance: meters over which it fades out where it meets a surface (no
 * hard edge where smoke crosses the road; 0: hard).
 */
struct ParticleEmitterSettings
{
	enum class BlendMode : int { Alpha = 0, Additive = 1 };

	bool Emitting = true;
	float Rate = 20.0f;              // particles a second
	float RatePerMeter = 0.0f;       // particles for each meter the emitter moves (added to Rate)
	uint32_t MaxParticles = 500;     // alive at once (the slice of the pool)
	float LifetimeMin = 1.5f;        // seconds
	float LifetimeMax = 2.5f;
	float SpeedMin = 0.5f;           // meters a second
	float SpeedMax = 1.5f;
	float ConeAngle = 25.0f;         // degrees around the entity's up
	float StartRadius = 0.0f;        // meters
	float InheritVelocity = 0.0f;    // the emitter's velocity times this
	glm::vec3 Acceleration = glm::vec3(0.0f, 0.3f, 0.0f);
	float Drag = 0.5f;
	float SizeStart = 0.3f;          // meters across
	float SizeEnd = 1.2f;
	glm::vec4 ColorStart = glm::vec4(0.8f, 0.8f, 0.8f, 0.6f);
	glm::vec4 ColorEnd = glm::vec4(0.8f, 0.8f, 0.8f, 0.0f);
	float SpinSpeed = 45.0f;         // degrees a second, random up to this either way
	std::string Texture;             // absolute here; in the scene file an asset reference. Empty: a soft round puff
	BlendMode Blend = BlendMode::Alpha;
	bool Lit = true;
	float SoftDistance = 0.5f;       // meters

	static constexpr uint32_t MaxParticlesLimit = 65536; // per emitter
	static const char* GetBlendName(BlendMode mode);
	static bool FindBlend(const std::string& name, BlendMode& mode);
};
