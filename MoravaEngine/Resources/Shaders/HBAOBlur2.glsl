#type vertex
#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(location = 0) out vec2 vs_TexCoords;

void main()
{
    vs_TexCoords = a_TexCoord;
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 430

const float KERNEL_RADIUS = 5.0;

layout(binding = 0) uniform sampler2D u_InputTex;

layout(push_constant) uniform Info
{
    vec2 InvResDirection;
    float Sharpness;
} u_Info;


layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 vs_TexCoords;

float BlurFunction(vec2 uv, float r, float centerC, float centerD, inout float wTotal)
{
    vec2  aoz = texture(u_InputTex, uv).xy;
    float c = aoz.x;
    float d = aoz.y;

    const float blurSigma = float(KERNEL_RADIUS) * 0.5;
    const float blurFalloff = 1.0 / (2.0 * blurSigma * blurSigma);

    float ddiff = (d - centerD) * u_Info.Sharpness;
    float w = exp2(-r * r * blurFalloff - ddiff * ddiff);
    wTotal += w;

    return c * w;
}

void main()
{
    vec2  aoz = texture(u_InputTex, vs_TexCoords).xy;
    float centerC = aoz.x;
    float centerD = aoz.y;

    float cTotal = centerC;
    float wTotal = 1.0;

    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = vs_TexCoords + u_Info.InvResDirection * r;
        cTotal += BlurFunction(uv, r, centerC, centerD, wTotal);
    }

    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        vec2 uv = vs_TexCoords - u_Info.InvResDirection * r;
        cTotal += BlurFunction(uv, r, centerC, centerD, wTotal);
    }

    outColor = vec4(cTotal / wTotal);
}
