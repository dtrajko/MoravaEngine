// Outline Shader
#version 450 core

layout (location = 0) in vec3  a_Position;
layout (location = 1) in vec3  a_Normal;
layout (location = 2) in vec3  a_Tangent;
layout (location = 3) in vec3  a_Binormal;
layout (location = 4) in vec2  a_TexCoord;
layout (location = 5) in ivec4 a_BoneIndices;
layout (location = 6) in vec4  a_BoneWeights;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

const int MAX_BONES = 100;
uniform mat4 u_BoneTransforms[MAX_BONES];

uniform bool u_Animated;


void main()
{
    if (u_Animated)
    {
        mat4 boneTransform = u_BoneTransforms[a_BoneIndices[0]] * a_BoneWeights[0];
        boneTransform += u_BoneTransforms[a_BoneIndices[1]] * a_BoneWeights[1];
        boneTransform += u_BoneTransforms[a_BoneIndices[2]] * a_BoneWeights[2];
        boneTransform += u_BoneTransforms[a_BoneIndices[3]] * a_BoneWeights[3];

        vec4 localPosition = boneTransform * vec4(a_Position, 1.0);

	    vec3 FragPos = (u_Transform * localPosition).xyz;
        gl_Position = u_ViewProjection * vec4(FragPos + a_Normal * 0.2, 1.0);
    }
    else
    {
	    // gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
	    vec3 FragPos = (u_Transform * vec4(a_Position, 1.0)).xyz;
        gl_Position = u_ViewProjection * vec4(FragPos + a_Normal * 0.2, 1.0);
    }
}
