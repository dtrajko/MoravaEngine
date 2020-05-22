#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

out vec2 vTexCoords;
out vec3 vNormal;
out vec3 vWorldPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vTexCoords = aTexCoords;
    vWorldPos = vec3(model * vec4(aPosition, 1.0));
    vNormal = mat3(model) * aNormal;   
    gl_Position =  projection * view * vec4(vWorldPos, 1.0);
}
