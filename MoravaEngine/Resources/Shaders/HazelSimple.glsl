#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 u_ViewProjectionMatrix;
};

layout (std140, binding = 1) uniform Transform
{
	mat4 u_Transform;
};

struct VertexOutput
{
    vec3 Normal;
	vec2 TexCoord;
};

layout (location = 0) out VertexOutput Output;

void main()
{
    Output.Normal = mat3(u_Transform) * a_Normal;
	Output.TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);

	gl_Position = u_ViewProjectionMatrix * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

struct VertexOutput
{
    vec3 Normal;
	vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;

void main()
{
	color = vec4(0.8, 0.2, 0.7, 1.0);
}
