// Present (Vulkan, SceneEnvMapVulkan): the finished image (the viewport composite, ViewportComposite.glsl) copied into the
// window's swapchain image, under the user interface. In the player, with no user interface, it is what the window shows.
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

layout (binding = 0) uniform sampler2D u_Texture; // the viewport composite: exposed, tonemapped, gamma corrected

void main()
{
	o_Color = vec4(texture(u_Texture, Input.TexCoord).rgb, 1.0);
}
