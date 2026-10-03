// Shadow map depth pass for skinned meshes (Vulkan, SceneEnvMapVulkan): same as ShadowDepth.glsl, with the vertex moved by the
// bone matrices of the current animation frame. The bone buffer is the mesh's own (set 2 of HazelPBR_Anim.glsl), bound here
// as set 0: the block is declared identically, so the descriptor set is compatible.
#type vertex
#version 450 core

// The mesh vertex (MeshH2M's AnimatedVertex)
layout(location = 0) in vec3 a_Position;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

const int MAX_BONES = 128;
layout (std140, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};

layout (push_constant) uniform Transform
{
	mat4 u_LightViewProjection; // the cascade's world -> shadow map clip space
	mat4 u_Model;
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
	gl_Position = u_LightViewProjection * u_Model * boneTransform * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

void main()
{
}
