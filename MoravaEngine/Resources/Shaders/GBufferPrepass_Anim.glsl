// G-buffer prepass for skinned meshes (see EnvMapVulkanGBuffer): the vertex stage of HazelPBR_Anim.glsl, the prepass's
// fragment stage (Include/GBufferPrepass_Fragment.glslh). Drawn with the HazelPBR_Anim pipeline's layout.
#type vertex
#version 450 core

#include "Include/MeshVertex_Anim.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/GBufferPrepass_Fragment.glslh"
