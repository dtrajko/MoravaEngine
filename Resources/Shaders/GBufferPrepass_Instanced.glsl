// G-buffer prepass for the foliage's copies (EnvMapVulkanFoliage): GBufferPrepass_Static.glsl with each copy's transform from
// the instance buffer (Include/MeshInstance.glslh; its previous transform is the same: the copies don't move). Drawn with the
// HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_MOTION
#define MESH_VERTEX_INSTANCED
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/GBufferPrepass_Fragment.glslh"
