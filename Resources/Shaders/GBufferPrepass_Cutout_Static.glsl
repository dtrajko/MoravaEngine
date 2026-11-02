// G-buffer prepass for static meshes with a cutout material (EnvMapVulkanMaterial::Surface::Cutout): GBufferPrepass_Static.glsl
// with the pixels where the albedo map's alpha is below the cutoff discarded, so they get no depth and the scene pass (depth
// EQUAL) shades what is behind them. Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_MOTION
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/GBufferPrepass_Fragment.glslh"
