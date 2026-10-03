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

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

#include "Include/FrameCamera.glslh"

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
	Output.WorldPosition = vec3(u_Transform * vec4(a_Position, 1.0));
    Output.Normal = mat3(u_Transform) * a_Normal;
	Output.TexCoord = a_TexCoord;//vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	Output.WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = a_Binormal;

	gl_Position = u_ViewProjectionMatrix * u_Transform * vec4(a_Position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/HazelPBR_Fragment.glslh"
