// G-buffer prepass for the foliage's copies with a cutout material (leaves): GBufferPrepass_Instanced.glsl with the pixels
// where the albedo map's alpha is below the cutoff discarded, as GBufferPrepass_Cutout_Static.glsl. Drawn with the
// HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_MOTION
#define MESH_VERTEX_INSTANCED
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MESH_FRAGMENT_INSTANCED

#define MATERIAL_CUTOUT
#include "Include/GBufferPrepass_Fragment.glslh"
