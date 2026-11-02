#version 330

layout (location = 0) in vec3 aPosition;

out vec2 vTexCoord1;
out vec2 vTexCoord2;
out float vBlendFactor;

struct TexCoordInfo {
	float numRows;
	float blendFactor;
};

uniform mat4 projection;
uniform mat4 modelView;

uniform vec2 texOffset1;
uniform vec2 texOffset2;
uniform TexCoordInfo texCoordInfo;

void main()
{
	vec2 texCoord = aPosition.xy + vec2(0.5, 0.5);
	texCoord.y = 1.0 - texCoord.y;
	texCoord /= texCoordInfo.numRows;
	vTexCoord1 = texCoord + texOffset1;
	vTexCoord2 = texCoord + texOffset2;
	vBlendFactor = texCoordInfo.blendFactor;

	gl_Position = projection * modelView * vec4(aPosition, 1.0);
}
