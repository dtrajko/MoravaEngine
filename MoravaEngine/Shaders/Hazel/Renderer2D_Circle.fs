// -----------------------------
// - Hazel 2D -
// Renderer2D Circle Shader
// -----------------------------

// #type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_Color2;

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat int v_EntityID;

void main()
{
	vec2 uv = vec2(0, 0);

	// Calculate distance and fill circle with white
	float distance = 1.0 - length(Input.LocalPosition);
	vec3 color = vec3(smoothstep(0.0, Input.Fade, distance));
	color *= vec3(smoothstep(Input.Thickness + Input.Fade, Input.Thickness, distance));

	// Set output color
	o_Color = vec4(color, 1.0);
	o_Color.rgb *= Input.Color;
}
