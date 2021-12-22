#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in mat4 aModel;
layout (location = 7) in vec4 aColor;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;
out vec4 vColor;

uniform mat4 projection;
uniform mat4 view;
uniform vec4 clipPlane;


void main()
{
    vec4 WorldPosition = aModel * vec4(aPosition, 1.0);
	gl_ClipDistance[0] = dot(WorldPosition, clipPlane);

    vTexCoord = aTexCoord;
    vNormal = aNormal;
    vFragPos = (aModel * vec4(aPosition, 1.0)).xyz;
    vColor = aColor;
    gl_Position = projection * view * aModel * vec4(aPosition, 1.0);
}
