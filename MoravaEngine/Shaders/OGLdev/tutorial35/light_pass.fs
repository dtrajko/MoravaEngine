#version 330 core

layout (location = 1) out vec4 DiffuseOut;

in vec2 TexCoords;

uniform sampler2D u_GBuffer_Position;
uniform sampler2D u_GBuffer_Diffuse;
uniform sampler2D u_GBuffer_Normal;
uniform sampler2D u_GBuffer_TexCoord;


void main()
{             
    DiffuseOut = texture(u_GBuffer_Diffuse, TexCoords);
}
