#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 fColor;

void main()
{
    vec2 pos = aPos * (gl_InstanceID / 100.0);
    gl_Position = projection * view * model * vec4(pos + aOffset, 0.0, 1.0);
    fColor = aColor;
}
