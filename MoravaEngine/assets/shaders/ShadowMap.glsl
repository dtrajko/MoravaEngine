// Shadow Map shader

#type vertex
#version 430

layout(location = 0) in vec3 a_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

void main()
{
	gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
}

#type fragment
#version 430

layout(location = 0) out vec4 o_Color;

void main()
{
}