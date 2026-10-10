// The probes of the probe volume, shown as small balls (SceneEnvMapVulkan, see EnvMapVulkanProbes): each ball is lit by
// its own probe only, so it shows what the probe holds: the light arriving there from every direction, as a white matte
// ball would show it. Drawn into the scene pass after the opaque meshes.
// No mesh: one square per probe (6 vertices, an instance per probe) that faces the camera; the fragment stage cuts the
// ball out of it and gives it the ball's depth.
// Set 0 is the per-frame set, declared as in the PBR shaders (Include/FrameCamera.glslh, Include/FrameSet.glslh).
#type vertex
#version 450 core

#include "Include/FrameCamera.glslh"

layout (push_constant) uniform Probes
{
	vec4 OriginRadius; // xyz: the position of probe (0, 0, 0); w: the balls' radius
	vec4 Spacing;      // xyz: from a probe to its neighbors
	ivec4 Counts;      // xyz: probes along each axis
	vec4 CameraRight;  // xyz: the camera's right and up directions in the world
	vec4 CameraUp;
} u_Probes;

layout (location = 0) out vec2 v_Local;                // the position in the square, -1..1
layout (location = 1) flat out ivec3 v_Probe;
layout (location = 2) flat out vec4 v_CenterRadius;
layout (location = 3) flat out vec3 v_CameraRight;
layout (location = 4) flat out vec3 v_CameraUp;
layout (location = 5) flat out mat4 v_ViewProjection;  // locations 5 to 8

const vec2 Corners[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main()
{
	// The instance index counts the probes layer by layer, row by row
	ivec3 counts = u_Probes.Counts.xyz;
	int index = gl_InstanceIndex;
	ivec3 probe = ivec3(index % counts.x, (index / counts.x) % counts.y, index / (counts.x * counts.y));
	vec3 center = u_Probes.OriginRadius.xyz + vec3(probe) * u_Probes.Spacing.xyz;

	vec2 corner = Corners[gl_VertexIndex];
	vec3 position = center + (corner.x * u_Probes.CameraRight.xyz + corner.y * u_Probes.CameraUp.xyz) * u_Probes.OriginRadius.w;

	v_Local = corner;
	v_Probe = probe;
	v_CenterRadius = vec4(center, u_Probes.OriginRadius.w);
	v_CameraRight = u_Probes.CameraRight.xyz;
	v_CameraUp = u_Probes.CameraUp.xyz;
	v_ViewProjection = u_ViewProjectionMatrix;
	gl_Position = u_ViewProjectionMatrix * vec4(position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

// The per-frame set 0 (with the probe atlases) and the probes' functions
#include "Include/ShadeSurface.glslh"

layout (location = 0) in vec2 v_Local;
layout (location = 1) flat in ivec3 v_Probe;
layout (location = 2) flat in vec4 v_CenterRadius;
layout (location = 3) flat in vec3 v_CameraRight;
layout (location = 4) flat in vec3 v_CameraUp;
layout (location = 5) flat in mat4 v_ViewProjection;

layout (location = 0) out vec4 o_Color;

void main()
{
	float distance2 = dot(v_Local, v_Local);
	if (distance2 > 1.0)
	{
		discard; // outside the ball's outline
	}
	// The ball's normal at this pixel (the camera is treated as far away: fine for a small ball)
	vec3 toCamera = normalize(cross(v_CameraRight, v_CameraUp));
	vec3 normal = normalize(v_Local.x * v_CameraRight + v_Local.y * v_CameraUp + sqrt(1.0 - distance2) * toCamera);

	// The ball's own depth, so it is hidden by (and hides) the scene as a ball would
	vec4 clip = v_ViewProjection * vec4(v_CenterRadius.xyz + normal * v_CenterRadius.w, 1.0);
	gl_FragDepth = clip.z / clip.w;

	// What the probe holds for the normal: the light arriving from that side, as a white matte surface sends it back.
	// A probe that isn't in use is shown dark red.
	vec3 irradiance = pow(ProbeStoredIrradiance(v_Probe, normal), vec3(u_ProbeSpacing.w));
	o_Color = ProbeData(v_Probe).w > 0.5 ? vec4(irradiance, 1.0) : vec4(0.25, 0.0, 0.0, 1.0);
}
