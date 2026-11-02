// Editor overlays for skinned meshes (Vulkan, SceneEnvMapVulkan): same as EditorOverlay.glsl, with the vertices
// moved by the bone matrices of the current animation frame (the mesh's own bone buffer, see HazelPBR_Anim.glsl)
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
// Not used: declared so the inputs match the mesh vertex layout (ModelH2M's AnimatedVertex)
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

const int MAX_BONES = 128;
layout (std140, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};

layout (push_constant) uniform Transform
{
	mat4 u_MVP; // view projection * model
};

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

	gl_Position = u_MVP * boneTransform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout (push_constant) uniform Settings
{
	layout (offset = 64) vec4 Color; // alpha 0 writes depth only
} u_Settings;

void main()
{
	o_Color = u_Settings.Color;
}
