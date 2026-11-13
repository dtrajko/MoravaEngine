// The PBR shader for the foliage's copies (EnvMapVulkanFoliage): HazelPBR_Static.glsl with each copy's transform from the
// instance buffer (Include/MeshInstance.glslh). For the scene pass, which tests its depth EQUAL against the instanced prepass's
// (GBufferPrepass_Instanced.glsl, GBufferPrepass_Cutout_Instanced.glsl: the same vertex stage), so a cutout material's holes
// need no variant here. Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_INSTANCED
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MESH_FRAGMENT_INSTANCED

#include "Include/HazelPBR_Fragment.glslh"
