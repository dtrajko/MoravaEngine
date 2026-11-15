// Particles, step 1 (SceneEnvMapVulkan, see EnvMapVulkanParticles): every particle of the pool a frame older. A living one
// is pulled by its emitter's acceleration, slowed by its drag, moved by its velocity and turned by its spin; one past its
// lifetime dies (Life 0). The frame's new particles start after this (ParticleSpawn.glsl), already moved for the part of
// the frame they lived.
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

layout (push_constant) uniform ParticleUpdatePush
{
	float DeltaTime;
	uint Count; // the pool's particles
} u_Update;

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= u_Update.Count)
	{
		return;
	}
	Particle particle = u_Particles[index];
	if (particle.VelocityLife.w <= 0.0)
	{
		return;
	}
	float dt = u_Update.DeltaTime;
	float age = particle.PositionAge.w + dt;
	if (age >= particle.VelocityLife.w)
	{
		u_Particles[index].VelocityLife.w = 0.0;
		return;
	}
	Emitter emitter = u_Emitters[uint(particle.Extra.w + 0.5)];
	vec3 velocity = particle.VelocityLife.xyz + emitter.AccelerationDrag.xyz * dt;
	velocity *= exp(-max(emitter.AccelerationDrag.w, 0.0) * dt);
	particle.PositionAge = vec4(particle.PositionAge.xyz + velocity * dt, age);
	particle.VelocityLife.xyz = velocity;
	particle.Extra.x += particle.Extra.y * dt;
	u_Particles[index] = particle;
}
