// Particles, step 2 (SceneEnvMapVulkan, see EnvMapVulkanParticles): an emitter's new particles of this frame, one thread
// each, into its slice of the pool where the ring's next ones go (the oldest ones' places). The i-th of n started at the
// point i / n of the frame: from that point of the emitter's way since the last frame (a fast emitter leaves an even
// trail, not a clump per frame), and it has lived the rest of the frame (moved for that time).
#type compute
#version 450 core

#include "Include/Particles.glslh"

layout (std430, binding = 0) buffer ParticlePool
{
	Particle u_Particles[];
};
layout (std430, binding = 1) readonly buffer ParticleEmitters
{
	Emitter u_Emitters[];
};

layout (push_constant) uniform ParticleSpawnPush
{
	float DeltaTime;
	uint Emitter; // its slot
	uint Seed;    // changes every frame
	uint Padding;
} u_Spawn;

const float Pi = 3.14159265358979;

// A unit vector within the cone around axis whose half angle has the cosine cosCone (evenly over the cone's cap)
vec3 ConeDirection(vec3 axis, float cosCone, inout uint seed)
{
	float cosTheta = mix(1.0, cosCone, ParticleRandom(seed));
	float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
	float phi = 2.0 * Pi * ParticleRandom(seed);
	vec3 helper = abs(axis.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(helper, axis));
	vec3 bitangent = cross(axis, tangent);
	return normalize(axis * cosTheta + (tangent * cos(phi) + bitangent * sin(phi)) * sinTheta);
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	Emitter emitter = u_Emitters[u_Spawn.Emitter];
	uint i = gl_GlobalInvocationID.x;
	uint count = emitter.Slice.w;
	uint sliceSize = emitter.Slice.y;
	if (i >= count || sliceSize == 0u)
	{
		return;
	}
	uint index = emitter.Slice.x + (emitter.Slice.z + i) % sliceSize;
	uint seed = ParticleHash(u_Spawn.Seed ^ ParticleHash(index * 9781u + 1u));

	// When in the frame it started (0: at its start, 1: at its end), and so where the emitter was then
	float when = (float(i) + ParticleRandom(seed)) / float(count);
	vec3 position = mix(emitter.PreviousPosition.xyz, emitter.Position.xyz, when);
	float radius = emitter.SpinRadius.y;
	if (radius > 0.0)
	{
		// Anywhere within the ball (the cube root: evenly through it, not crowded at its center)
		vec3 offsetDirection = ConeDirection(vec3(0.0, 1.0, 0.0), -1.0, seed);
		position += offsetDirection * radius * pow(ParticleRandom(seed), 1.0 / 3.0);
	}

	float speed = mix(emitter.SpeedLife.x, emitter.SpeedLife.y, ParticleRandom(seed));
	vec3 velocity = ConeDirection(normalize(emitter.AxisCone.xyz), emitter.AxisCone.w, seed) * speed +
		emitter.VelocityInherit.xyz * emitter.VelocityInherit.w;
	float life = max(mix(emitter.SpeedLife.z, emitter.SpeedLife.w, ParticleRandom(seed)), 0.001);
	float spin = (ParticleRandom(seed) * 2.0 - 1.0) * emitter.SpinRadius.x;
	float turn = ParticleRandom(seed) * 2.0 * Pi;

	// The rest of the frame it lived: moved and turned for it (without the acceleration's small share)
	float age = (1.0 - when) * u_Spawn.DeltaTime;
	Particle particle;
	particle.PositionAge = vec4(position + velocity * age, age);
	particle.VelocityLife = vec4(velocity, life);
	particle.Extra = vec4(turn + spin * age, spin, ParticleRandom(seed), float(u_Spawn.Emitter));
	particle.Light = vec4(1.0); // a lit one's: the next light pass (this frame, after this)
	u_Particles[index] = particle;
}
