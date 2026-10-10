// Probe update, step 1 for skinned meshes (SceneEnvMapVulkan, see EnvMapVulkanProbes): as ProbeCapture_Static.glsl, with
// the vertex moved by the bone matrices of the current animation frame (set 2: the model's bone buffer, as in
// HazelPBR_Anim.glsl).
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's AnimatedVertex): only what the capture reads
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 5) in ivec4 a_BoneIndices;
layout(location = 6) in vec4 a_BoneWeights;

#include "Include/FrameCamera.glslh"
#include "Include/ProbeCapture.glslh"

// Declared as in Include/MeshVertex_Anim.glslh, so the model's bone descriptor set fits
const int MAX_BONES = 128;
layout (std140, set = 2, binding = 0) uniform BoneTransforms
{
	mat4 u_BoneTransforms[MAX_BONES];
	mat4 u_PreviousBoneTransforms[MAX_BONES];
};

layout (location = 0) out vec3 v_WorldPosition;
layout (location = 1) out vec3 v_Normal;

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
	mat4 transform = CaptureTransform() * boneTransform;
	vec4 worldPosition = transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;
	v_Normal = mat3(transform) * a_Normal;
	gl_Position = u_Capture.FaceViewProjection * worldPosition;
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/ProbeCapture_Fragment.glslh"
