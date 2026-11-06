// G-buffer prepass for a terrain's chunks (see EnvMapVulkanGBuffer, EnvMapVulkanTerrain): GBufferPrepass_Static.glsl with the
// terrain's surface (Include/TerrainMaterial.glslh), so its normals and roughness are the ones HazelPBR_Terrain.glsl lights.
// Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

#define MESH_VERTEX_MOTION
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#define MATERIAL_TERRAIN
#include "Include/GBufferPrepass_Fragment.glslh"
