#version 330 core

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D albedoMap;
uniform vec4 tintColor;

void main()
{
    FragColor = texture(albedoMap, vTexCoord) * tintColor;
}
