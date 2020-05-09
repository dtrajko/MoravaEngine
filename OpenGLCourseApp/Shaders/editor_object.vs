#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec4 vColor;
out vec2 vTexCoord;
out vec3 vNormal;
out vec3 vFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vTexCoord = aTexCoord;
	vNormal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPosition, 1.0);
    vColor = vec4(clamp(aPosition, 0.0f, 1.0f), 1.0f);
}
