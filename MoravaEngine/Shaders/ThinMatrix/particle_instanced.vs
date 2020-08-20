#version 330

layout (location = 0) in vec3 aPosition;
layout (location = 1) in mat4 modelView;
layout (location = 5) in vec4 texOffsets;
layout (location = 6) in float blendFactor;

out vec2 vTexCoord1;
out vec2 vTexCoord2;
out float vBlendFactor;


uniform float numberOfRows;
uniform mat4 projection;

void main()
{
	vec2 texCoord = aPosition.xy + vec2(0.5, 0.5);
	texCoord.y = 1.0 - texCoord.y;
	texCoord /= numberOfRows;
	vTexCoord1 = texCoord + texOffsets.xy;
	vTexCoord2 = texCoord + texOffsets.zw;
	vBlendFactor = blendFactor;

	gl_Position = projection * modelView * vec4(aPosition, 1.0);
}
