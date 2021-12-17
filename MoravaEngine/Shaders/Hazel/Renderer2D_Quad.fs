// #type fragment
#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out int o_EntityID;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;
in flat int v_EntityID;

uniform vec4 u_Color;
uniform float u_TilingFactor;
uniform sampler2D u_Textures[32];

void main()
{
	// color = texture(u_Textures[int(v_TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;
	color = v_Color;

	o_EntityID = v_EntityID;
}
