#version 330

in vec4 v_Color;
in vec2 v_TexCoord;

out vec4 color;

uniform sampler2D u_Texture;

void main()
{
	color = texture(u_Texture, v_TexCoord);
}
