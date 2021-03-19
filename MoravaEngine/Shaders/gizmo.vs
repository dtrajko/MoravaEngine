#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
}
