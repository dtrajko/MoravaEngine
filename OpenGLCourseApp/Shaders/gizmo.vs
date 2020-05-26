#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vPosition;
out vec3 vFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    vPosition = vec3(model * vec4(aPosition, 1.0));
    vTexCoord = aTexCoord;
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vFragPos = (model * vec4(aPosition, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
