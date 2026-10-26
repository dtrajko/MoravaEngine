// The PBR shader for static meshes with a cutout material (EnvMapVulkanMaterial::Surface::Cutout): HazelPBR_Static.glsl with
// the pixels where the albedo map's alpha is below the cutoff discarded. For the passes that draw the meshes without the
// G-buffer prepass's depth (the water's reflection); the scene pass needs no variant: its depth EQUAL test keeps only what
// the prepass left. Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/HazelPBR_Fragment.glslh"
