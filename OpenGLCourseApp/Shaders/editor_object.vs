#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 vFragPos;
out vec4 vDirLightSpacePos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 dirLightTransform;
uniform vec4 clipPlane;

void main()
{
    vPosition = vec3(model * vec4(aPosition, 1.0));
    vTexCoord = aTexCoord;

    vec4 WorldPosition = model * vec4(aPosition, 1.0);
	gl_ClipDistance[0] = dot(WorldPosition, clipPlane);

    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vFragPos = (model * vec4(aPosition, 1.0)).xyz;
	vDirLightSpacePos = dirLightTransform * model * vec4(aPosition, 1.0);
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
