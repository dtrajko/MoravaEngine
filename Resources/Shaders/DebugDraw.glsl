// Debug drawings in the world (SceneEnvMapVulkan, see EnvMapVulkanDebugDraw): triangles, lines and dots of the debug
// views (the Navigation view), drawn into the editor overlay after tonemapping and hidden by the scene: each pixel is
// tested here against the scene's depth, and what the scene hides is drawn with a fraction of its alpha.
//
// No vertex buffer: the primitives are vec4s in a buffer (set 1), positions in xyz and the colors' bits in w. A
// triangle is three vec4s (vertex 3 x i + 0..2); a line two (its ends; the second's w is its width in pixels) and a dot
// two (its center; the radius in pixels, the outline's color), each made here of two triangles facing the screen
// (vertex 6 x i + 0..5), a pixel wider than they are for their soft edges.
// Colors are 0xAABBGGRR; alpha 0 picks one of the frame's 64 palette colors by its low six bits (set 0).
// The output is premultiplied (the pipeline blends "over" that way, as the viewport composite reads the overlay).
#type vertex
#version 450 core

layout (std430, set = 0, binding = 1) readonly buffer DebugPalette
{
	uint u_Palette[64];
};

layout (std430, set = 1, binding = 0) readonly buffer DebugPrimitives
{
	vec4 u_Primitives[];
};

layout (push_constant) uniform DebugDrawPush
{
	mat4 ViewProjection;
	uvec4 Draw;     // x: the draw's first vec4, y: 0 triangles, 1 lines, 2 dots
	vec4 Viewport;  // xy: the size in pixels, z: the hidden parts' alpha, w: the depth tolerance (meters)
	vec4 Camera;    // xyz: its position, w: the draw distance (0: no limit)
	vec4 Depth;     // xy: the projection's [2][2] and [3][2]
} u_Push;

layout (location = 0) out vec4 v_Color;
layout (location = 1) out vec3 v_WorldPosition;
layout (location = 2) out vec3 v_Shape;        // lines: x across in pixels, y half the width; dots: xy from the center in pixels, z the radius
layout (location = 3) flat out uint v_Kind;
layout (location = 4) flat out vec4 v_Outline; // dots

vec4 DecodeColor(float bits)
{
	uint color = floatBitsToUint(bits);
	if ((color >> 24u) == 0u)
	{
		color = u_Palette[color & 63u];
	}
	return unpackUnorm4x8(color);
}

// A point in clip space moved by pixels on the screen
vec4 OffsetPixels(vec4 clip, vec2 pixels)
{
	return vec4(clip.xy + pixels * 2.0 / u_Push.Viewport.xy * clip.w, clip.zw);
}

const vec4 Outside = vec4(2.0, 2.0, 2.0, 1.0);

void main()
{
	uint vertex = uint(gl_VertexIndex);
	uint kind = u_Push.Draw.y;
	v_Kind = kind;
	v_Shape = vec3(0.0);
	v_Outline = vec4(0.0);
	if (kind == 0u)
	{
		uint first = u_Push.Draw.x + (vertex / 3u) * 3u;
		vec3 position = u_Primitives[first + vertex % 3u].xyz;
		v_Color = DecodeColor(u_Primitives[first].w);
		v_WorldPosition = position;
		gl_Position = u_Push.ViewProjection * vec4(position, 1.0);
		return;
	}

	// The quad's corner: x along (0 the start, 1 the end), y across (-1, 1)
	const vec2 Corners[6] = vec2[6](vec2(0.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(0.0, -1.0), vec2(1.0, 1.0), vec2(0.0, 1.0));
	vec2 corner = Corners[vertex % 6u];
	uint first = u_Push.Draw.x + (vertex / 6u) * 2u;
	vec4 a = u_Primitives[first];
	vec4 b = u_Primitives[first + 1u];
	v_Color = DecodeColor(a.w);

	if (kind == 1u)
	{
		// A line: its ends in clip space, the one behind the camera brought to just in front of it
		vec4 clipA = u_Push.ViewProjection * vec4(a.xyz, 1.0);
		vec4 clipB = u_Push.ViewProjection * vec4(b.xyz, 1.0);
		const float Near = 0.01;
		if (clipA.w < Near && clipB.w < Near)
		{
			gl_Position = Outside;
			return;
		}
		vec3 worldA = a.xyz, worldB = b.xyz;
		if (clipA.w < Near)
		{
			float t = (Near - clipA.w) / (clipB.w - clipA.w);
			clipA = mix(clipA, clipB, t);
			worldA = mix(worldA, worldB, t);
		}
		else if (clipB.w < Near)
		{
			float t = (Near - clipB.w) / (clipA.w - clipB.w);
			clipB = mix(clipB, clipA, t);
			worldB = mix(worldB, worldA, t);
		}
		vec2 screenA = clipA.xy / clipA.w * 0.5 * u_Push.Viewport.xy;
		vec2 screenB = clipB.xy / clipB.w * 0.5 * u_Push.Viewport.xy;
		vec2 along = screenB - screenA;
		along = dot(along, along) > 1e-8 ? normalize(along) : vec2(1.0, 0.0);
		vec2 across = vec2(-along.y, along.x);
		float halfWidth = max(b.w, 0.5) * 0.5;
		float reach = halfWidth + 1.0; // a pixel more for the soft edge
		vec4 clip = corner.x < 0.5 ? clipA : clipB;
		// The ends reach out as far as the sides, so lines meeting at a corner leave no gap
		gl_Position = OffsetPixels(clip, across * corner.y * reach + along * (corner.x * 2.0 - 1.0) * reach);
		v_WorldPosition = corner.x < 0.5 ? worldA : worldB;
		v_Shape = vec3(corner.y * reach, halfWidth, 0.0);
		return;
	}

	// A dot: a square around its center, as wide as the disc and its outline
	vec4 clip = u_Push.ViewProjection * vec4(a.xyz, 1.0);
	if (clip.w < 0.01)
	{
		gl_Position = Outside;
		return;
	}
	float radius = b.x;
	float reach = radius + 2.5;
	vec2 square = vec2(corner.x * 2.0 - 1.0, corner.y);
	gl_Position = OffsetPixels(clip, square * reach);
	v_WorldPosition = a.xyz;
	v_Shape = vec3(square * reach, radius);
	v_Outline = DecodeColor(b.y);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

layout (set = 0, binding = 0) uniform sampler2D u_SceneDepth;

layout (push_constant) uniform DebugDrawPush
{
	mat4 ViewProjection;
	uvec4 Draw;
	vec4 Viewport;
	vec4 Camera;
	vec4 Depth;
} u_Push;

layout (location = 0) in vec4 v_Color;
layout (location = 1) in vec3 v_WorldPosition;
layout (location = 2) in vec3 v_Shape;
layout (location = 3) flat in uint v_Kind;
layout (location = 4) flat in vec4 v_Outline;

layout (location = 0) out vec4 o_Color;

// A depth to the distance along the view direction (the projection's two depth terms)
float ToDistance(float depth)
{
	return u_Push.Depth.y / (depth + u_Push.Depth.x);
}

void main()
{
	vec4 color = v_Color;
	if (v_Kind == 1u)
	{
		// A line's soft sides
		color.a *= clamp(v_Shape.y + 0.5 - abs(v_Shape.x), 0.0, 1.0);
	}
	else if (v_Kind == 2u)
	{
		// A dot: the disc, its outline around it, soft at the edge
		float distance = length(v_Shape.xy);
		float radius = v_Shape.z;
		float outline = v_Outline.a > 0.0 ? 1.5 : 0.0;
		vec4 disc = mix(color, v_Outline, clamp(distance - radius + 0.5, 0.0, 1.0) * (outline > 0.0 ? 1.0 : 0.0));
		disc.a *= clamp(radius + outline + 0.5 - distance, 0.0, 1.0);
		color = disc;
	}

	// The draw distance, faded over its last tenth
	if (u_Push.Camera.w > 0.0)
	{
		float distance = length(v_WorldPosition - u_Push.Camera.xyz);
		color.a *= clamp((u_Push.Camera.w - distance) / (u_Push.Camera.w * 0.1), 0.0, 1.0);
	}

	// Hidden by the scene: behind its surface by more than the tolerance (the sky hides nothing)
	ivec2 size = textureSize(u_SceneDepth, 0);
	ivec2 texel = clamp(ivec2(gl_FragCoord.xy * vec2(size) / u_Push.Viewport.xy), ivec2(0), size - 1);
	float sceneDepth = texelFetch(u_SceneDepth, texel, 0).r;
	if (sceneDepth < 1.0)
	{
		float scene = ToDistance(sceneDepth);
		float here = ToDistance(gl_FragCoord.z);
		if (here > scene * 1.002 + u_Push.Viewport.w)
		{
			color.a *= u_Push.Viewport.z;
		}
	}
	if (color.a <= 0.002)
	{
		discard;
	}
	o_Color = vec4(color.rgb * color.a, color.a);
}
