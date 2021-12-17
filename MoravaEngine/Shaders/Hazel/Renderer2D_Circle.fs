// -----------------------------
// - Hazel 2D -
// Renderer2D Circle Shader
// -----------------------------

// #type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;
// layout(location = 1) out vec4 o_Color2; // output to 2nd color attachment of the render framebuffer

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
	float Fade;
};

layout (location = 0) in VertexOutput Input;
layout (location = 4) in flat int v_EntityID;

void main()
{
	// Calculate distance and fill circle with white
	float distance = 1.0 - length(Input.LocalPosition);
	float circle = smoothstep(0.0, Input.Fade, distance);
	circle *= smoothstep(Input.Thickness + Input.Fade, Input.Thickness, distance);

	if (circle == 0.0)
	{
		discard;
	}

	// Set output color
	o_Color = Input.Color;
	o_Color.a *= circle;

	o_EntityID = v_EntityID;
	// o_Color2 = vec4(0.0, 1.0, 1.0, 1.0); // output to 2nd color attachment of the render framebuffer
}
