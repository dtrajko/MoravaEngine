// Render texture composite (Vulkan, SceneEnvMapVulkan, 7.5 of docs/engine/Game_Engine_Runtime_Guide.html): a texture
// camera's linear HDR view, exposed, tonemapped and gamma corrected as the viewport composite does it (ViewportComposite.glsl),
// for a render texture of the Color format (a monitor, a minimap). No bloom, overlays or debug views: what a screen in the
// game shows.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	Output.TexCoord = a_TexCoord;
	gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D u_Texture; // the texture camera's view: linear HDR

layout(push_constant) uniform Uniforms
{
	float Exposure;
	float HuePreservation; // 0: ACES per channel, 1: hue-preserving (see Tonemap)
} u_Uniforms;

#include "Include/Tonemap.glslh"

void main()
{
	const float gamma = 2.2;
	vec3 color = texture(u_Texture, Input.TexCoord).rgb * u_Uniforms.Exposure;
	o_Color = vec4(pow(Tonemap(color, u_Uniforms.HuePreservation), vec3(1.0 / gamma)), 1.0);
}
