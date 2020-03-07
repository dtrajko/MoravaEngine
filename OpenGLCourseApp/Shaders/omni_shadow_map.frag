#version 330

in vec4 FragPos;

uniform vec3 lightPosition;
uniform float farPlane;


void main()
{
	float distance = length(FragPos.xyz - lightPosition);
	distance = distance / farPlane;
	gl_FragDepth = distance;
}
