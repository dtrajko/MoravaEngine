// Outline Shader

#type vertex
#version 430

layout(location = 0) in vec3 a_Position;

layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

uniform mat4 u_ViewProjection;
uniform mat4 u_Transform;

const int MAX_BONES = 100;
uniform mat4 u_BoneTransforms[100];

void main()
{
	mat4 boneTransform = u_BoneTransforms[a_BoneIndices[0]] * a_BoneWeights[0];
    boneTransform += u_BoneTransforms[a_BoneIndices[1]] * a_BoneWeights[1];
    boneTransform += u_BoneTransforms[a_BoneIndices[2]] * a_BoneWeights[2];
    boneTransform += u_BoneTransforms[a_BoneIndices[3]] * a_BoneWeights[3];

	vec4 localPosition = boneTransform * vec4(a_Position, 1.0);
	gl_Position = u_ViewProjection * u_Transform * localPosition;
}

#type fragment
#version 430

layout(location = 0) out vec4 color;

void main()
{
	color = vec4(1.0, 0.5, 0.0, 1.0);
}