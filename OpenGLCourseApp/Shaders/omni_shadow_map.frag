#version 330

in vec4 FragPos;

uniform vec3 lightPos;
uniform float farPlane;


void main()
{
	float distance = length(FragPos.xyz - lightPos);
	distance = distance / farPlane;
	gl_FragDepth = distance;
}
