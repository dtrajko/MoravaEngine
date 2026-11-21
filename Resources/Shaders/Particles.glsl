// Particles, drawn (SceneEnvMapVulkan, see EnvMapVulkanParticles): one square per particle of an emitter's slice, facing
// the camera, sized and colored by the particle's age, turned by its spin. A dead particle's square is put outside the view.
// A stretched emitter's particles are streaks along their motion (tracers, sparks); a view model emitter's are drawn with
// the view models' camera (the first person's muzzle flash: their field of view, their depth pressed toward the eye).
// The fragment stage: the emitter's texture (its alpha the shape), or a soft round puff without one; times the light the
// particle gets (lit emitters: ParticleLight.glsl lights each particle once a frame, at its center: a big puff covers many
// pixels, often on top of each other), or as it is; faded where it meets the scene's surfaces (the soft distance) and by
// the distance haze.
//
// Set 0 is the per-frame set, declared as in the PBR shaders (Include/FrameCamera.glslh, Include/FrameSet.glslh through
// Include/ShadeSurface.glslh). Set 1: the pool, the emitters and the scene's depth. Set 2: the emitter's texture.
#type vertex
#version 450 core

#include "Include/FrameCamera.glslh"
#include "Include/Particles.glslh"

layout (std430, set = 1, binding = 0) readonly buffer ParticlePool
{
	Particle u_Particles[];
};
layout (std430, set = 1, binding = 1) readonly buffer ParticleEmitters
{
	Emitter u_Emitters[];
};

layout (push_constant) uniform ParticleDrawPush
{
	vec4 CameraRight;   // xyz: the camera's right in the world
	vec4 CameraUp;      // xyz: its up
	vec4 CameraForward; // xyz: where it looks
	vec4 DepthParams;   // x: projection[2][2], y: projection[3][2], z: projection[2][3] (the depth buffer to the view distance)
	uvec4 Emitter;      // x: the emitter's slot
} u_Draw;

layout (location = 0) out vec2 v_Corner;        // -1..1 across the square
layout (location = 1) out vec4 v_Color;
layout (location = 2) out vec3 v_WorldPosition;
layout (location = 3) out float v_ViewDistance; // along the camera's view direction
layout (location = 4) flat out vec4 v_SoftFlags; // x: the soft distance; y: the flags; z: the particle's random number
layout (location = 5) flat out vec3 v_Light;     // the light it gets (ParticleLight.glsl), for a lit emitter

const vec2 Corners[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main()
{
	Particle particle = u_Particles[gl_InstanceIndex];
	Emitter emitter = u_Emitters[u_Draw.Emitter.x];
	if (particle.VelocityLife.w <= 0.0)
	{
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // outside the view: nothing drawn
		return;
	}
	float t = clamp(particle.PositionAge.w / particle.VelocityLife.w, 0.0, 1.0);
	float size = mix(emitter.SizeSoftFlags.x, emitter.SizeSoftFlags.y, t);
	vec2 corner = Corners[gl_VertexIndex];
	vec3 center = particle.PositionAge.xyz;
	vec3 position;
	float stretch = emitter.SpinRadius.w;
	vec3 velocity = particle.VelocityLife.xyz;
	float streak = stretch * length(velocity);
	if (stretch > 0.0 && streak > 1e-4)
	{
		// A streak from where it was Stretch seconds ago (not before it started) to where it is, its size across; the
		// square's y along it (a cap of half the size at each end), x across it as the camera sees it. No spin
		vec3 along = velocity / length(velocity);
		float trail = min(streak, length(velocity) * particle.PositionAge.w);
		vec3 tail = center - along * trail;
		vec3 toCamera = normalize(u_CameraWorldPosition.xyz - center);
		vec3 across = cross(along, toCamera);
		across = length(across) > 1e-4 ? normalize(across) : u_Draw.CameraRight.xyz;
		vec3 end = corner.y > 0.0 ? center + along * (0.5 * size) : tail - along * (0.5 * size);
		position = end + across * (corner.x * 0.5 * size);
	}
	else
	{
		float c = cos(particle.Extra.x), s = sin(particle.Extra.x);
		vec2 turned = vec2(corner.x * c - corner.y * s, corner.x * s + corner.y * c);
		position = center + (turned.x * u_Draw.CameraRight.xyz + turned.y * u_Draw.CameraUp.xyz) * (0.5 * size);
	}
	uint flags = EmitterFlags(emitter);
	bool viewModel = (flags & EmitterViewModel) != 0u;

	v_Corner = corner;
	v_Color = mix(emitter.ColorStart, emitter.ColorEnd, t);
	v_WorldPosition = position;
	v_ViewDistance = dot(position - u_CameraWorldPosition.xyz, u_Draw.CameraForward.xyz);
	// The view models' depth is pressed toward the eye: no soft fade against the scene's (it would compare unlike depths)
	v_SoftFlags = vec4(viewModel ? 0.0 : emitter.SizeSoftFlags.z, emitter.SizeSoftFlags.w, particle.Extra.z, stretch > 0.0 && streak > 1e-4 ? 1.0 : 0.0);
	v_Light = particle.Light.rgb;
	gl_Position = (viewModel ? u_ViewModelViewProjectionMatrix : u_ViewProjectionMatrix) * vec4(position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

// The per-frame set 0 (lights, shadows, the environment, the probes) and the lighting's functions
#include "Include/ShadeSurface.glslh"
#include "Include/Haze.glslh"
#include "Include/Particles.glslh"

layout (set = 1, binding = 2) uniform sampler2D u_SceneDepth;
layout (set = 2, binding = 0) uniform sampler2D u_Texture;

layout (push_constant) uniform ParticleDrawPush
{
	vec4 CameraRight;
	vec4 CameraUp;
	vec4 CameraForward;
	vec4 DepthParams;
	uvec4 Emitter;
} u_Draw;

layout (location = 0) in vec2 v_Corner;
layout (location = 1) in vec4 v_Color;
layout (location = 2) in vec3 v_WorldPosition;
layout (location = 3) in float v_ViewDistance;
layout (location = 4) flat in vec4 v_SoftFlags;
layout (location = 5) flat in vec3 v_Light;

layout (location = 0) out vec4 o_Color;

// The depth buffer's value as the distance along the camera's view direction (as Include/SSAO.glslh's LinearDepth)
float SceneViewDistance(float depth)
{
	float viewZ = u_Draw.DepthParams.y / (depth * u_Draw.DepthParams.z - u_Draw.DepthParams.x);
	return viewZ * u_Draw.DepthParams.z;
}

void main()
{
	uint flags = uint(v_SoftFlags.y + 0.5);
	vec2 uv = v_Corner * 0.5 + 0.5;
	vec4 color = v_Color;
	if ((flags & EmitterTextured) != 0u)
	{
		color *= texture(u_Texture, vec2(uv.x, 1.0 - uv.y));
	}
	else
	{
		// A soft round puff: full in the middle, fading smoothly to nothing at the edge (a streak: soft across it and at its
		// ends, full along it)
		float r2 = v_SoftFlags.w > 0.5 ? v_Corner.x * v_Corner.x + pow(abs(v_Corner.y), 8.0) : dot(v_Corner, v_Corner);
		float puff = clamp(1.0 - r2, 0.0, 1.0);
		color.a *= puff * puff;
	}

	// Faded where it meets a surface: by how far the surface behind it is, over the soft distance
	float softDistance = v_SoftFlags.x;
	if (softDistance > 0.0)
	{
		float scene = SceneViewDistance(texelFetch(u_SceneDepth, ivec2(gl_FragCoord.xy), 0).r);
		color.a *= clamp((scene - v_ViewDistance) / softDistance, 0.0, 1.0);
	}
	if (color.a <= 0.002)
	{
		discard;
	}

	if ((flags & EmitterLit) != 0u)
	{
		color.rgb *= v_Light; // once per particle, at its center (ParticleLight.glsl)
	}

	if ((flags & EmitterAdditive) != 0u)
	{
		// Added light: the haze dims it with distance
		color.rgb *= color.a * (1.0 - HazeAmount(u_CameraPosition, v_WorldPosition));
		o_Color = vec4(color.rgb, 0.0);
	}
	else
	{
		color.rgb = ApplyHaze(color.rgb, u_CameraPosition, v_WorldPosition);
		o_Color = color;
	}
}
