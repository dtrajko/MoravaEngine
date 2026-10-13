// Compute skinning for ray tracing (SceneEnvMapVulkan, see EnvMapVulkanRayTracing): a skinned model's vertices, moved by
// its bones, written into a buffer. The mesh shaders skin in their vertex stage, so the moved vertices exist nowhere
// in memory; an acceleration structure is built from vertices in a buffer, and a ray hit reads them there.
// One dispatch per skinned model and frame, before the model's acceleration structures are refitted to the new pose.
// The same sum of bone matrices as Include/MeshVertex_Anim.glslh. The model's transform is not applied: it is the
// instance's, in the top-level structure.
#type compute
#version 450 core
#extension GL_EXT_scalar_block_layout : require

// H2M::AnimatedVertex, 88 bytes (scalar layout: no padding)
struct SkinningSourceVertex
{
	vec3 Position;
	vec3 Normal;
	vec3 Tangent;
	vec3 Binormal;
	vec2 TexCoord;
	ivec4 BoneIndices;
	vec4 BoneWeights;
};

// H2M::VertexH2M, 56 bytes: RayTracingVertex in Include/RayTracing.glslh
struct SkinnedVertex
{
	vec3 Position;
	vec3 Normal;
	vec3 Tangent;
	vec3 Binormal;
	vec2 TexCoord;
};

const int MAX_BONES = 128;
// The model's bone buffer (set 2 of its mesh shader): this frame's matrices come first
layout (std140, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};
layout (scalar, binding = 1) readonly buffer SourceVertices
{
	SkinningSourceVertex u_SourceVertices[];
};
layout (scalar, binding = 2) writeonly buffer SkinnedVertices
{
	SkinnedVertex o_SkinnedVertices[];
};

layout (push_constant) uniform SkinningPush
{
	ivec4 Counts; // x = the model's vertices
} u_Skinning;

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main()
{
	uint index = gl_GlobalInvocationID.x;
	if (index >= uint(u_Skinning.Counts.x))
	{
		return;
	}
	SkinningSourceVertex source = u_SourceVertices[index];

	// Vertices without bone weights (parts of the model that are not attached to the skeleton) keep their shape
	mat4 boneTransform = mat4(1.0);
	if (dot(source.BoneWeights, vec4(1.0)) > 0.0001)
	{
		ivec4 indices = min(source.BoneIndices, ivec4(MAX_BONES - 1));
		boneTransform  = u_BoneTransforms[indices.x] * source.BoneWeights.x;
		boneTransform += u_BoneTransforms[indices.y] * source.BoneWeights.y;
		boneTransform += u_BoneTransforms[indices.z] * source.BoneWeights.z;
		boneTransform += u_BoneTransforms[indices.w] * source.BoneWeights.w;
	}

	SkinnedVertex skinned;
	skinned.Position = vec3(boneTransform * vec4(source.Position, 1.0));
	skinned.Normal = mat3(boneTransform) * source.Normal;
	skinned.Tangent = mat3(boneTransform) * source.Tangent;
	skinned.Binormal = mat3(boneTransform) * source.Binormal;
	skinned.TexCoord = source.TexCoord;
	o_SkinnedVertices[index] = skinned;
}
