#version 440

layout (location = 0) in vec3 aPosition;

out vec2 vTexCoord1;
out vec2 vTexCoord2;
out float vBlendFactor;

uniform mat4 modelView;
uniform vec4 texOffsets;
uniform float blendFactor;

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
