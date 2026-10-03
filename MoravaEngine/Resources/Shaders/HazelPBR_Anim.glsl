// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated.
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)
// Descriptor sets (Vulkan), ordered by how often they change (see VulkanShaderH2M::FrameDescriptorSet):
// - set 0, per frame:    Camera, SceneData (camera position, environment rotation), environment maps, BRDF LUT, Lights,
//                        the sun's shadow map and its cascades
// - set 1, per material: the material's texture maps
// - set 2, per object:   bone matrices (HazelPBR_Anim.glsl only)
// Push constants: the transform (vertex stage) and the material values (fragment stage).
#type vertex
#version 450 core

// Skinned version of HazelPBR_Static.glsl (Vulkan). Both share the fragment stage (Include/HazelPBR_Fragment.glslh),
// so materials, texture slots and the Material Editor work the same for animated meshes.

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

#include "Include/FrameCamera.glslh"

// Final bone matrices of the current animation frame (ModelH2M::GetBoneTransforms), updated every frame
// Set 2, per object: final bone matrices of the current animation frame (ModelH2M::GetBoneTransforms), updated every frame
const int MAX_BONES = 128;
layout (std140, set = 2, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
};

layout (push_constant) uniform Transform
{
	mat4 u_Transform;
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	// Vertices without bone weights (parts of the model that are not attached to the skeleton) keep their shape
	mat4 boneTransform = mat4(1.0);
	if (dot(a_BoneWeights, vec4(1.0)) > 0.0001)
	{
		ivec4 indices = min(a_BoneIndices, ivec4(MAX_BONES - 1));
		boneTransform  = u_BoneTransforms[indices.x] * a_BoneWeights.x;
		boneTransform += u_BoneTransforms[indices.y] * a_BoneWeights.y;
		boneTransform += u_BoneTransforms[indices.z] * a_BoneWeights.z;
		boneTransform += u_BoneTransforms[indices.w] * a_BoneWeights.w;
	}

	mat4 skinnedTransform = u_Transform * boneTransform;

	Output.WorldPosition = vec3(skinnedTransform * vec4(a_Position, 1.0));
	Output.Normal = mat3(skinnedTransform) * a_Normal;
	Output.TexCoord = a_TexCoord;
	// The tangent frame moves with the bones too (the normal map would be lit wrong on moving parts otherwise)
	Output.WorldNormals = mat3(skinnedTransform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = mat3(boneTransform) * a_Binormal;

	gl_Position = u_ViewProjectionMatrix * skinnedTransform * vec4(a_Position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/HazelPBR_Fragment.glslh"
