#version 330

in vec4 vFragPos;

uniform vec3 lightPosition;
uniform float farPlane;


void main()
{
	float distance = length(vFragPos.xyz - lightPosition);
	distance = distance / farPlane;
	gl_FragDepth = distance;
}
