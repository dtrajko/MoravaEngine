// Shadow map depth pass for skinned meshes with a cutout material: ShadowDepth_Cutout.glsl with the vertex moved by the bone
// matrices of the current animation frame. Drawn with the HazelPBR_Anim pipeline's layout, so the bone buffer is set 2,
// as in the PBR shader (declared here with its first array only).
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's AnimatedVertex)
layout(location = 0) in vec3 a_Position;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

const int MAX_BONES = 128;
layout (std140, set = 2, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};

layout (push_constant) uniform Transform
{
	mat4 u_LightModelViewProjection;
};

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	// Vertices without bone weights keep their shape (as in HazelPBR_Anim.glsl)
	mat4 boneTransform = mat4(1.0);
	if (dot(a_BoneWeights, vec4(1.0)) > 0.0001)
	{
		ivec4 indices = min(a_BoneIndices, ivec4(MAX_BONES - 1));
		boneTransform  = u_BoneTransforms[indices.x] * a_BoneWeights.x;
		boneTransform += u_BoneTransforms[indices.y] * a_BoneWeights.y;
		boneTransform += u_BoneTransforms[indices.z] * a_BoneWeights.z;
		boneTransform += u_BoneTransforms[indices.w] * a_BoneWeights.w;
	}
	v_TexCoord = a_TexCoord;
	gl_Position = u_LightModelViewProjection * boneTransform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/MeshMaterial.glslh"

layout (location = 0) in vec2 v_TexCoord;

void main()
{
	MaterialCutAway(MaterialTexCoord(v_TexCoord));
}
