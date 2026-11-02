// Shadow map viewer (Vulkan, SceneEnvMapVulkan): turns the depth of a spot light's shadow map, or of a point light's 6 cube
// faces, into a grayscale image for the Lights panel. The depth is converted back to the distance from the light (darker:
// closer to the light): perspective depth is close to 1 almost everywhere, so shown as it is the map looks blank.
#type compute
#version 450 core

layout(binding = 0) uniform sampler2DArray u_ShadowMap;               // every layer of the shadow map, no comparison
layout(binding = 1, rgba8) restrict writeonly uniform image2D o_View; // 4:3, tiles of a third of its height

layout(push_constant) uniform Settings
{
	int Mode;      // 0: one layer (a spot light), a square on the left; 1: a cube, unfolded into a cross of 6 tiles
	int BaseLayer; // the spot light's layer, or the cube's first face (+X)
	float Near;    // of the light's shadow maps
	float Far;
} u_Settings;

// The cross: -X, +Z, +X, -Z in the middle row (looking around the horizon), +Y above and -Y below +Z.
// Cube faces are stored in the layer order +X, -X, +Y, -Y, +Z, -Z.
int CrossFace(ivec2 cell)
{
	if (cell.y == 1)
	{
		const int row[4] = int[4](1, 4, 0, 5);
		return row[cell.x];
	}
	if (cell.x == 1)
	{
		return cell.y == 0 ? 2 : 3;
	}
	return -1;
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(o_View);
	if (pixel.x >= size.x || pixel.y >= size.y)
	{
		return;
	}
	const vec4 background = vec4(0.1, 0.1, 0.1, 1.0);

	int layer;
	vec2 uv;
	if (u_Settings.Mode == 0)
	{
		if (pixel.x >= size.y)
		{
			imageStore(o_View, pixel, background);
			return;
		}
		uv = (vec2(pixel) + 0.5) / float(size.y);
		layer = u_Settings.BaseLayer;
	}
	else
	{
		int tile = size.y / 3;
		ivec2 cell = pixel / tile;
		int face = CrossFace(cell);
		if (face < 0)
		{
			imageStore(o_View, pixel, background);
			return;
		}
		uv = (vec2(pixel - cell * tile) + 0.5) / float(tile);
		layer = u_Settings.BaseLayer + face;
	}

	float depth = textureLod(u_ShadowMap, vec3(uv, float(layer)), 0.0).r;
	// Perspective depth (perspectiveRH_ZO) -> distance along the view direction
	float n = u_Settings.Near, f = u_Settings.Far;
	float distance = n * f / (f - depth * (f - n));
	float gray = depth >= 1.0 ? 1.0 : clamp(distance / f, 0.0, 1.0);
	imageStore(o_View, pixel, vec4(vec3(gray), 1.0));
}
