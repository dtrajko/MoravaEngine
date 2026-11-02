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

layout(location = 0) out vec4 outColor;

struct VertexOutput
{
	vec2 TexCoords;
};
layout (location = 0) in VertexOutput Input;

layout(binding = 0) uniform sampler2D u_Texture;

float ScreenDistance(vec2 v, vec2 texelSize)
{
    float ratio = texelSize.x / texelSize.y;
    v.x /= ratio;
    return dot(v, v);
}

void main()
{
    vec4 color = texture(u_Texture, Input.TexCoords);

    ivec2 texSize = textureSize(u_Texture, 0);
    vec2 texelSize = vec2(1.0f / float(texSize.x), 1.0f / float(texSize.y));

    vec4 result;
    result.xy = vec2(100, 100);
    result.z = ScreenDistance(result.xy, texelSize);
    // Inside vs. outside
    result.w = color.a > 0.5f ? 1.0f : 0.0f;
    outColor = result;
}