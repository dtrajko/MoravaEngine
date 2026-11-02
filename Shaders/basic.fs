#version 330 core

out vec4 FragColor;

in vec4 vColor;

// uniform vec4 tintColor;

void main()
{
    // FragColor = vColor * tintColor;
    FragColor = vColor;
}
