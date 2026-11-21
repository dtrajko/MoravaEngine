// Decals, drawn (SceneEnvMapVulkan, see EnvMapVulkanDecals; Shooter E6): bullet holes, scorch marks, posters. Each decal is
// its box (a unit cube from -0.5 to 0.5 placed by its transform), drawn by its back faces without a depth test (so it shows
// with the camera inside it too); each pixel takes the scene's surface there (the depth, back to the world), and when that
// is inside the box, the texture at its place across the box (x and z) is multiplied into the scene's color (the
// pipeline's blend: 1 leaves it, the decal's color darkens it as a darker surface would be: the light on it stays).
// Surfaces turned away from the box's up fade out (no smear down a wall's side), and so do the box's ends along its up.
// The sky and the first person's view models (their depth pressed toward the eye) get none.
//
// Set 0 is the per-frame set, declared as in the PBR shaders (Include/FrameCamera.glslh, Include/FrameSet.glslh through
// Include/ShadeSurface.glslh). Set 1: the decals, the scene's depth and its G-buffer normals. Set 2: the decals' texture.
#type vertex
#version 450 core

#include "Include/FrameCamera.glslh"

struct Decal
{
	mat4 World;    // the box: a unit cube placed
	mat4 Inverse;  // the world into the box's space
	vec4 Color;    // rgb: times the texture; a: how much it covers (its fade too)
	vec4 Params;   // x: 1 textured (0: a soft round spot)
};

layout (std430, set = 1, binding = 0) readonly buffer Decals
{
	Decal u_Decals[];
};

layout (location = 0) flat out uint v_Decal;
layout (location = 1) flat out uint v_Away; // 1: the face is a back face (turned away from the camera): the one drawn

// The cube's 36 corners (12 triangles), each face wound counterclockwise seen from outside
const vec3 CubeCorners[8] = vec3[8](vec3(-0.5, -0.5, -0.5), vec3(0.5, -0.5, -0.5), vec3(0.5, 0.5, -0.5), vec3(-0.5, 0.5, -0.5),
	vec3(-0.5, -0.5, 0.5), vec3(0.5, -0.5, 0.5), vec3(0.5, 0.5, 0.5), vec3(-0.5, 0.5, 0.5));
const uint CubeIndices[36] = uint[36](
	4u, 5u, 6u, 4u, 6u, 7u,  // +z
	1u, 0u, 3u, 1u, 3u, 2u,  // -z
	5u, 1u, 2u, 5u, 2u, 6u,  // +x
	0u, 4u, 7u, 0u, 7u, 3u,  // -x
	7u, 6u, 2u, 7u, 2u, 3u,  // +y
	0u, 1u, 5u, 0u, 5u, 4u); // -y

const vec3 FaceNormals[6] = vec3[6](vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, -1.0), vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0), vec3(0.0, -1.0, 0.0));

void main()
{
	Decal decal = u_Decals[gl_InstanceIndex];
	vec3 corner = CubeCorners[CubeIndices[gl_VertexIndex]];
	vec4 world = decal.World * vec4(corner, 1.0);
	// Each pixel of the box once: its back faces only (decided here, whatever way the projection winds them; with the
	// camera inside the box every face is a back face)
	vec3 normal = transpose(mat3(decal.Inverse)) * FaceNormals[gl_VertexIndex / 6];
	v_Away = dot(normal, world.xyz - u_CameraWorldPosition.xyz) > 0.0 ? 1u : 0u;
	v_Decal = uint(gl_InstanceIndex);
	gl_Position = u_ViewProjectionMatrix * world;
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

// The per-frame set 0 (the camera, the haze) and the functions the haze needs
#include "Include/ShadeSurface.glslh"
#include "Include/Haze.glslh"

struct Decal
{
	mat4 World;
	mat4 Inverse;
	vec4 Color;
	vec4 Params;
};

layout (std430, set = 1, binding = 0) readonly buffer Decals
{
	Decal u_Decals[];
};
layout (set = 1, binding = 1) uniform sampler2D u_SceneDepth;
layout (set = 1, binding = 2) uniform sampler2D u_SceneNormals;
layout (set = 2, binding = 0) uniform sampler2D u_Texture;

layout (push_constant) uniform DecalDrawPush
{
	mat4 InverseViewProjection; // the frame's, as drawn (the jitter too): a pixel and its depth back to the world
	vec4 ViewSize;              // xy: the pixels across and down; zw: their inverses
} u_Draw;

layout (location = 0) flat in uint v_Decal;
layout (location = 1) flat in uint v_Away;

layout (location = 0) out vec4 o_Color;

// The view models' depth is pressed under this (Include/SSAO.glslh's ViewModelDepthMax)
const float ViewModelDepthMax = 0.1;

void main()
{
	if (v_Away == 0u)
	{
		discard; // a front face: its back face behind it draws this pixel
	}
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	float depth = texelFetch(u_SceneDepth, pixel, 0).r;
	if (depth >= 1.0 || depth < ViewModelDepthMax)
	{
		discard; // the sky; the first person's arms and gun
	}
	vec2 ndc = (vec2(pixel) + 0.5) * u_Draw.ViewSize.zw * 2.0 - 1.0;
	vec4 world = u_Draw.InverseViewProjection * vec4(ndc, depth, 1.0);
	vec3 worldPosition = world.xyz / world.w;

	Decal decal = u_Decals[v_Decal];
	vec3 local = (decal.Inverse * vec4(worldPosition, 1.0)).xyz;
	if (any(greaterThan(abs(local), vec3(0.5))))
	{
		discard;
	}

	// Turned away from the box's up: fading out from 70 degrees to 85 (a hole on a wall's corner stays on its face)
	vec3 up = normalize(decal.World[1].xyz);
	vec3 normal = texelFetch(u_SceneNormals, pixel, 0).xyz;
	float facing = dot(normal, normal) > 1e-6 ? dot(normalize(normal), up) : 1.0;
	float fade = smoothstep(0.08, 0.34, facing);
	// And toward the box's ends along its up
	fade *= 1.0 - smoothstep(0.35, 0.5, abs(local.y));

	vec2 uv = vec2(local.x, -local.z) + 0.5;
	vec4 color = decal.Color;
	if (decal.Params.x > 0.5)
	{
		color *= texture(u_Texture, uv);
	}
	else
	{
		// A soft round spot
		vec2 offset = uv * 2.0 - 1.0;
		float spot = clamp(1.0 - dot(offset, offset), 0.0, 1.0);
		color.a *= spot * spot;
	}
	color.a *= fade * (1.0 - HazeAmount(u_CameraPosition, worldPosition));
	if (color.a <= 0.002)
	{
		discard;
	}
	o_Color = vec4(mix(vec3(1.0), color.rgb, clamp(color.a, 0.0, 1.0)), 1.0);
}
