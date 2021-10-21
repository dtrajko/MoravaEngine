// Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.

#type vertex
#version 430 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

void main()
{
    gl_Position = vec4(a_Position.xy, 0.0, 1.0);
}


#type fragment
#version 430

layout(binding = 0) uniform sampler2D u_LinearDepthTex;

layout(std140, binding = 17) uniform ScreenData
{
    vec2 u_InvFullResolution;
    vec2 u_FullResolution;
};

layout(push_constant) uniform Info
{
    vec2 UVOffset;
} u_Info;


layout(location = 0) out vec4 out_Color[8];

//----------------------------------------------------------------------------------

#if 1
void main() {
    vec2 uv = floor(gl_FragCoord.xy) * 4.0 + u_Info.UVOffset + 0.5;
    uv *= u_InvFullResolution;

    vec4 S0 = textureGather(u_LinearDepthTex, uv, 0);
    vec4 S1 = textureGatherOffset(u_LinearDepthTex, uv, ivec2(2, 0), 0);

    out_Color[0] = vec4(S0.w, 0.0, 0.0, 1.0);
    out_Color[1] = vec4(S0.z, 0.0, 0.0, 1.0);
    out_Color[2] = vec4(S1.w, 0.0, 0.0, 1.0);
    out_Color[3] = vec4(S1.z, 0.0, 0.0, 1.0);
    out_Color[4] = vec4(S0.x, 0.0, 0.0, 1.0);
    out_Color[5] = vec4(S0.y, 0.0, 0.0, 1.0);
    out_Color[6] = vec4(S1.x, 0.0, 0.0, 1.0);
    out_Color[7] = vec4(S1.y, 0.0, 0.0, 1.0);
}
#else
void main() {
    vec2 uv = floor(gl_FragCoord.xy) * 4.0 + u_Info.UVOffset;
    ivec2 tc = ivec2(uv);

    out_Color[0] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(0, 0)).x;
    out_Color[1] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(1, 0)).x;
    out_Color[2] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(2, 0)).x;
    out_Color[3] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(3, 0)).x;
    out_Color[4] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(0, 1)).x;
    out_Color[5] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(1, 1)).x;
    out_Color[6] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(2, 1)).x;
    out_Color[7] = texelFetchOffset(u_LinearDepthTex, tc, 0, ivec2(3, 1)).x;
}

#endif