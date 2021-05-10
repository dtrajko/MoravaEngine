// Outline Shader
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;


void main()
{
	// gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);

	vec3 FragPos = (u_Transform * vec4(a_Position, 1.0)).xyz;
	gl_Position = u_ViewProjection * vec4(FragPos + a_Normal * 0.3, 1.0);
}
