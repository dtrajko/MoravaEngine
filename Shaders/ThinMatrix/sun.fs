#version 330

in vec2 pass_textureCoords;

out vec4 out_color;

uniform sampler2D sunTexture;

void main(void){

	out_color = texture(sunTexture, pass_textureCoords);

}
