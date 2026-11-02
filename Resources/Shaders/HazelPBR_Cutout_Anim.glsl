// The PBR shader for skinned meshes with a cutout material: HazelPBR_Anim.glsl with the pixels where the albedo map's alpha
// is below the cutoff discarded (see HazelPBR_Cutout_Static.glsl). Drawn with the HazelPBR_Anim pipeline's layout.
#type vertex
#version 450 core

#include "Include/MeshVertex_Anim.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/HazelPBR_Fragment.glslh"
