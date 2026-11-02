// Caustics (SceneEnvMapVulkan, see EnvMapVulkanWater.h): the pattern of light the waves focus on what is under the water.
// A grid over the water surface, one vertex per sample of the waves: each vertex follows the sun's ray through the surface,
// bent by the wave normal there (Snell's law), down to the focus depth; the triangles are drawn where the rays land, with
// additive blending. A triangle that the waves squeezed into a smaller area brings its light there: the light per texel is
// the surface area that lands on it (1 where the waves are flat), so bright lines form where the rays cross and dark gaps
// where they spread apart.
// The map is laid out by where the sun's ray enters the water, so the PBR shaders look it up at the point where the
// sun's ray to a fragment entered (see WaterCaustics in Include/WaterVolume.glslh).
#type vertex
#version 450 core

layout (set = 0, binding = 0) uniform sampler2D u_WaterNormalMap; // x in R, up in B, z in G (as in Water.glsl)
// Binding 1: the water's settings (the same buffer as the water's), for the Gerstner waves
#define WATER_SETTINGS_SET 0
#include "Include/WaterSettings.glslh"

layout (push_constant) uniform Caustics
{
	vec4 u_Region;      // the map: xy = its corner in the world (min x, min z), z = its size (square), w = its resolution (texels)
	vec4 u_Grid;        // the grid: xy = its corner (the map's with a margin around it), z = the cell size, w = cells per side
	vec4 u_RippleOffsets; // normal map layer 1 offset in xy, layer 2 in zw (as the water's u_WaveOffsets)
	vec4 u_Ripples;       // x, y = world size of a normal map tile (layer 1, 2), z = wave strength, w = unused
	vec4 u_Sun;         // xyz = toward the sun, w = the focus depth (meters under the surface)
	vec4 u_Lod;         // x, y = the normal map mip level of layer 1, 2 (the waves smaller than a grid cell are left out)
};

layout (location = 0) out vec2 v_SourceTexel;

const float WaterIndexOfRefraction = 1.33;

// The wave normal, as in Water.glsl (the swell's tilt plus the two normal map layers'), the ripples at a mip level that
// matches the grid. The swell moves the surface too; its focusing comes from its tilt, the small shift of the point is left out.
vec3 WaveNormal(vec2 worldXZ)
{
	vec3 a = textureLod(u_WaterNormalMap, worldXZ / u_Ripples.x + u_RippleOffsets.xy, u_Lod.x).rbg * 2.0 - 1.0; // x, up, z
	vec3 b = textureLod(u_WaterNormalMap, worldXZ / u_Ripples.y + u_RippleOffsets.zw, u_Lod.y).rbg * 2.0 - 1.0;
	vec2 slope = (a.xz / max(a.y, 0.1) + b.xz / max(b.y, 0.1)) * u_Ripples.z + GerstnerTilt(worldXZ);
	return normalize(vec3(slope.x, 1.0, slope.y));
}

// Where a ray traveling in direction travel from the surface is, depth meters under it (relative to where it entered)
vec2 Descend(vec3 travel, float depth)
{
	return travel.xz * (depth / max(-travel.y, 0.05));
}

void main()
{
	// Two triangles per grid cell, from the vertex index (no vertex buffer)
	const vec2 corners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 1.0), vec2(0.0, 0.0));
	int cellsPerSide = int(u_Grid.w);
	int cell = gl_VertexIndex / 6;
	vec2 gridPoint = vec2(cell % cellsPerSide, cell / cellsPerSide) + corners[gl_VertexIndex % 6];
	vec2 worldXZ = u_Grid.xy + gridPoint * u_Grid.z;

	// The sun's ray through the wavy surface, compared with the ray through a flat one: the map holds the light by the flat
	// ray's entry point, so the waves only move the light around it
	vec3 sunIn = -u_Sun.xyz;
	vec3 flatTravel = refract(sunIn, vec3(0.0, 1.0, 0.0), 1.0 / WaterIndexOfRefraction);
	vec3 travel = refract(sunIn, WaveNormal(worldXZ), 1.0 / WaterIndexOfRefraction);
	vec2 landing = worldXZ + Descend(travel, u_Sun.w) - Descend(flatTravel, u_Sun.w);

	// The vertex's place on the surface, in map texels (its derivatives in the fragment shader measure the area that lands
	// on a texel), and where its light lands, in the map
	vec2 sourceUV = (worldXZ - u_Region.xy) / u_Region.z;
	v_SourceTexel = sourceUV * u_Region.w;
	vec2 landingUV = (landing - u_Region.xy) / u_Region.z;
	gl_Position = vec4(landingUV * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 450 core

layout (location = 0) in vec2 v_SourceTexel;

layout (location = 0) out vec4 o_Light;

void main()
{
	// The surface area (in texels) whose light lands on this texel: the triangle's area before the waves bent its rays,
	// per its area here. 1 under flat water; capped where the rays meet in a point (the area there goes to zero, and the
	// light of such a sliver of a triangle would land on a single texel).
	vec2 dx = dFdx(v_SourceTexel);
	vec2 dy = dFdy(v_SourceTexel);
	float area = abs(dx.x * dy.y - dx.y * dy.x);
	o_Light = vec4(min(area, 12.0), 0.0, 0.0, 1.0);
}
