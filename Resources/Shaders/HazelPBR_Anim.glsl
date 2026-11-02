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
// so materials, texture slots and the Material Editor work the same for animated meshes. The vertex stage is shared with
// the G-buffer prepass (GBufferPrepass_Anim.glsl): bit-identical positions for the depth EQUAL test.
#include "Include/MeshVertex_Anim.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/HazelPBR_Fragment.glslh"
