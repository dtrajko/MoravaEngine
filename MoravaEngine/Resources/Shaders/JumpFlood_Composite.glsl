#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoords;

struct VertexOutput
{
	vec2 TexCoords;
};
layout (location = 0) out VertexOutput Output;

void main()
{
    Output.TexCoords = a_TexCoords;

    gl_Position = vec4(a_Position, 1.0f);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

struct VertexOutput
{
	vec2 TexCoords;
};
layout (location = 0) in VertexOutput Input;

layout(binding = 0) uniform sampler2D u_Texture;

void main()
{
    vec4 pixel = texture(u_Texture, Input.TexCoords);

    // Signed distance (squared)
    float dist = sqrt(pixel.z);
    float alpha = smoothstep(0.004f, 0.002f, dist);
    if (alpha == 0.0)
        discard;

    vec3 outlineColor = vec3(1.0f, 0.5f, 0.0f);
    o_Color = vec4(outlineColor, alpha);
}