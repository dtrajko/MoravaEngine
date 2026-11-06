// The PBR shader for a terrain's chunks (EnvMapVulkanTerrain): HazelPBR_Static.glsl with the terrain's surface, its three
// layers mixed by height and slope (Include/TerrainMaterial.glslh) instead of one material. For the scene pass (depth
// EQUAL, after GBufferPrepass_Terrain.glsl) and the passes without the prepass (the water's reflection, render textures).
// Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_TERRAIN
#include "Include/HazelPBR_Fragment.glslh"
