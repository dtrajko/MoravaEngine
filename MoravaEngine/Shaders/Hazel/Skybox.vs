// Skybox shader

// #type vertex
#version 430

layout(location = 0) in vec3 a_Position;

uniform mat4 u_InverseVP;

out vec3 v_Position;

void main()
{
	vec4 position = vec4(a_Position.xy, 1.0, 1.0);
	gl_Position = position;

	v_Position = (u_InverseVP * position).xyz;
}
