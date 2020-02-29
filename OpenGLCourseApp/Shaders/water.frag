#version 330

in vec2 textureCoords;

out vec4 out_Color;

uniform sampler2D reflectionTexture;
uniform sampler2D refractionTexture;


void main(void)
{
	vec4 reflectColor = texture(reflectionTexture, textureCoords);
	vec4 refractColor = texture(refractionTexture, textureCoords);

	out_Color = mix(reflectColor, refractColor, 0.5);
}
