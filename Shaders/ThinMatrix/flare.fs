#version 330

in vec2 pass_textureCoords;

out vec4 out_colour;

uniform sampler2D flareTexture;
uniform float brightness;

void main(void){

    out_colour = texture(flareTexture, pass_textureCoords);
    out_colour.a *= brightness;

}
