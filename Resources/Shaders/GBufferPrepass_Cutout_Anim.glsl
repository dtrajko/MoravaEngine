// G-buffer prepass for skinned meshes with a cutout material: GBufferPrepass_Anim.glsl with the pixels where the albedo
// map's alpha is below the cutoff discarded (see GBufferPrepass_Cutout_Static.glsl). Drawn with the HazelPBR_Anim
// pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_MOTION
#include "Include/MeshVertex_Anim.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/GBufferPrepass_Fragment.glslh"
