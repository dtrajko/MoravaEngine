#version 330

layout (location = 0) in vec2 aPosition;

out vec4 clipSpace;
out vec2 textureCoords;
out vec3 toCameraVector;
out vec3 fromLightVector;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;

const float tiling = 4.0;

void main()
{
	vec4 worldPosition = model * vec4(aPosition.x, 0.0, aPosition.y, 1.0);
	clipSpace = projection * view * model * vec4(aPosition.x, 0.0, aPosition.y, 1.0);
	gl_Position = clipSpace;
	textureCoords = vec2(aPosition.x / 2.0 + 0.5, aPosition.y / 2.0 + 0.5) * tiling;
	toCameraVector = cameraPosition - worldPosition.xyz;
	fromLightVector = worldPosition.xyz - lightPosition;
}
