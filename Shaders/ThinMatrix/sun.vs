#version 330

in vec2 in_position;

out vec2 pass_textureCoords;

uniform mat4 mvpMatrix;

void main(void) {

	pass_textureCoords = in_position + vec2(0.5, 0.5);
	pass_textureCoords.y = 1.0 - pass_textureCoords.y;
	gl_Position = mvpMatrix * vec4(in_position, 0.0, 1.0);

}
