#type vertex
#version 430

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_TexCoord;


void main()
{
	gl_Position = vec4(a_Position, 1.0);
}


#type fragment
#version 430


layout(binding = 0)  uniform sampler2DArray u_TexResultsArray;

layout(location = 0) out vec4 out_Color;


//----------------------------------------------------------------------------------

void main() {
	ivec2 fullResPos = ivec2(gl_FragCoord.xy);
	ivec2 offset = fullResPos & 3;
	int sliceId = offset.y * 4 + offset.x;
	ivec2 quarterResPos = fullResPos >> 2;

	out_Color = vec4(texelFetch(u_TexResultsArray, ivec3(quarterResPos, sliceId), 0).xy, 0, 1.0);

}