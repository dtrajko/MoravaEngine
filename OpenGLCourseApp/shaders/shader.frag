#version 330

in vec4 vCol;
in vec2 TexCoord;

out vec4 color;

uniform sampler2D theTexture;

void main()
{
	color = texture(theTexture, TexCoord);
}
