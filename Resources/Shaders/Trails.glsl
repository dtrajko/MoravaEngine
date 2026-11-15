// Trails, drawn (SceneEnvMapVulkan, see EnvMapVulkanTrails): skid marks. Each piece of ribbon between two points of a trail
// is two triangles, made here from the points (no vertex buffer): vertex 6 x i + 0..5 are piece i's corners. A point not
// joined to the one before it starts a new ribbon: the piece before it is put outside the view.
// The fragment stage: the mark's color (times its texture along the ribbon, or a plain band with soft edges), faded by
// the distance haze, and multiplied into the scene (the pipeline's blend): 1 leaves the surface as it is, the mark's color
// darkens it as a darker surface would be (the light on it stays).
//
// Set 0 is the per-frame set, declared as in the PBR shaders (Include/FrameCamera.glslh, Include/FrameSet.glslh through
// Include/ShadeSurface.glslh). Set 1: the points of all trails. Set 2: the trail's texture.
#type vertex
#version 450 core

#include "Include/FrameCamera.glslh"

struct TrailPoint
{
	vec4 PositionConnected; // xyz: on the surface (lifted a little); w: 1 joined to the point before
	vec4 SideAlong;         // xyz: half the ribbon's width, sideways; w: meters along the ribbon
	vec4 Color;
};

layout (std430, set = 1, binding = 0) readonly buffer TrailPoints
{
	TrailPoint u_Points[];
};

layout (push_constant) uniform TrailDrawPush
{
	uvec4 Trail;   // x: its first point; y: 1 textured
	vec4 Texture;  // x: the meters along the ribbon a texture covers once
} u_Draw;

layout (location = 0) out vec2 v_UV;  // x: along the ribbon (texture lengths), y: across it, 0..1
layout (location = 1) out vec4 v_Color;
layout (location = 2) out vec3 v_WorldPosition;

// x: the piece's first point (0) or its second (1); y: the side (-1, 1)
const vec2 Corners[6] = vec2[6](vec2(0.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(0.0, -1.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main()
{
	uint piece = uint(gl_VertexIndex) / 6u;
	vec2 corner = Corners[uint(gl_VertexIndex) % 6u];
	uint first = u_Draw.Trail.x + piece;
	if (u_Points[first + 1u].PositionConnected.w < 0.5)
	{
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // a new ribbon starts at the second point: nothing between them
		return;
	}
	TrailPoint point = u_Points[first + uint(corner.x)];
	vec3 position = point.PositionConnected.xyz + point.SideAlong.xyz * corner.y;

	v_UV = vec2(point.SideAlong.w / max(u_Draw.Texture.x, 0.01), corner.y * 0.5 + 0.5);
	v_Color = point.Color;
	v_WorldPosition = position;
	gl_Position = u_ViewProjectionMatrix * vec4(position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

// The per-frame set 0 (the camera, the haze) and the functions the haze needs
#include "Include/ShadeSurface.glslh"
#include "Include/Haze.glslh"

layout (set = 2, binding = 0) uniform sampler2D u_Texture;

layout (push_constant) uniform TrailDrawPush
{
	uvec4 Trail;
	vec4 Texture;
} u_Draw;

layout (location = 0) in vec2 v_UV;
layout (location = 1) in vec4 v_Color;
layout (location = 2) in vec3 v_WorldPosition;

layout (location = 0) out vec4 o_Color;

void main()
{
	vec4 color = v_Color;
	if (u_Draw.Trail.y != 0u)
	{
		color *= texture(u_Texture, v_UV);
	}
	else
	{
		// A plain band, its edges soft (a fifth of the width each side)
		float edge = 1.0 - abs(v_UV.y * 2.0 - 1.0);
		color.a *= smoothstep(0.0, 0.4, edge);
	}
	// Far away the haze covers the road and the mark alike
	color.a *= 1.0 - HazeAmount(u_CameraPosition, v_WorldPosition);
	if (color.a <= 0.002)
	{
		discard;
	}
	o_Color = vec4(mix(vec3(1.0), color.rgb, clamp(color.a, 0.0, 1.0)), 1.0);
}
