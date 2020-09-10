#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(aPosition.x, aPosition.y, 0.0, 1.0); 
    TexCoords = aTexCoords;
}
