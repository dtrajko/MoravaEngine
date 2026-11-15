// Particles, step 3 (SceneEnvMapVulkan, see EnvMapVulkanParticles): the light each living particle of a lit emitter gets
// this frame, at its center, once (the draw multiplies its pixels by it: a big puff covers many pixels, often on top of
// each other, so lighting each pixel would cost many times more). A cloud of small bits scatters light every way, so it
// is lit as a white surface that faces both the light and the open sky would be: the sun through its shadow, and the
// environment's light from above.
//
// Its own small lookups, not Include/Shadows.glslh's: a compute shader has no implicit level of detail (those use the
// fragment stage's lookups with a bias). The sun's shadow is a 3 x 3 filter of the cascade the particle is in. Not in:
// tinted glass's shadows, the probes' light (the environment's instead), the water's.
//
// Set 0: the per-frame set's lights, shadows and environment (Include/FrameSet.glslh), copied from the frame's set (its
// layout is this compute shader's own). Set 1: the pool and the emitters.
#type compute
#version 450 core

#include "Include/FrameSet.glslh"
#include "Include/Particles.glslh"

layout (std430, set = 1, binding = 0) buffer ParticlePool
{
	Particle u_Particles[];
};
layout (std430, set = 1, binding = 1) readonly buffer ParticleEmitters
{
	Emitter u_Emitters[];
};

layout (push_constant) uniform ParticleLightPush
{
	uint Count; // the pool's particles
} u_Light;

const float InvPi = 0.31830988618;

vec3 RotateAboutY(float degrees, vec3 v)
{
	float angle = radians(degrees);
	float c = cos(angle), s = sin(angle);
	return vec3(c * v.x - s * v.z, v.y, s * v.x + c * v.z);
}

// 1 lit, 0 in the sun's shadow; 1 beyond the shadow distance (fading out before it, as the meshes' shadows do)
float ParticleSunShadow(vec3 P)
{
	if (u_ShadowsEnabled < 0.5)
	{
		return 1.0;
	}
	float viewDistance = dot(P - u_CameraPosition, u_CameraForward);
	float shadowDistance = u_CascadeSplits[3];
	if (viewDistance >= shadowDistance || viewDistance < 0.0)
	{
		return 1.0;
	}
	int cascade = 3;
	for (int i = 0; i < 3; i++)
	{
		if (viewDistance < u_CascadeSplits[i])
		{
			cascade = i;
			break;
		}
	}
	vec4 clip = u_CascadeViewProjection[cascade] * vec4(P, 1.0);
	vec3 coord = clip.xyz / clip.w;
	if (coord.z >= 1.0)
	{
		return 1.0;
	}
	vec2 uv = coord.xy * 0.5 + 0.5;
	vec2 step = vec2(max(u_ShadowSoftness, 1.0) * u_ShadowMapTexelSize);
	float lit = 0.0;
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			lit += textureGrad(u_ShadowMap, vec4(uv + vec2(x, y) * step, float(cascade), coord.z), vec2(0.0), vec2(0.0));
		}
	}
	lit /= 9.0;
	float fadeStart = shadowDistance * 0.9;
	return mix(lit, 1.0, clamp((viewDistance - fadeStart) / (shadowDistance - fadeStart), 0.0, 1.0));
}

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= u_Light.Count)
	{
		return;
	}
	Particle particle = u_Particles[index];
	if (particle.VelocityLife.w <= 0.0)
	{
		return;
	}
	Emitter emitter = u_Emitters[uint(particle.Extra.w + 0.5)];
	if ((EmitterFlags(emitter) & EmitterLit) == 0u)
	{
		return;
	}
	vec3 P = particle.PositionAge.xyz;
	// The environment's light from above (u_EnvIrradianceTex: the irradiance divided by pi, the diffuse convention)
	vec3 light = textureLod(u_EnvIrradianceTex, RotateAboutY(u_EnvMapRotation, vec3(0.0, 1.0, 0.0)), 0.0).rgb;
	if (u_Sun.Intensity > 0.0)
	{
		light += u_Sun.Color * u_Sun.Intensity * InvPi * ParticleSunShadow(P);
	}
	u_Particles[index].Light = vec4(light, 1.0);
}
