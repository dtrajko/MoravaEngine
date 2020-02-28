#version 400 core

out vec4 out_Color;

uniform vec3 eyePosition;

void main(void) {

	out_Color = vec4(eyePosition, 1.0);

}
