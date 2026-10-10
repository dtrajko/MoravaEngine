// G-buffer prepass for static meshes (see EnvMapVulkanGBuffer): the vertex stage of HazelPBR_Static.glsl, a fragment stage
// that writes the shading normal, the roughness and the motion. Drawn with the HazelPBR_Static pipeline's layout.
#type vertex
#version 450 core

// The prepass also needs where each vertex was in the previous frame (the PBR shaders leave this out)
#define MESH_VERTEX_MOTION
#include "Include/MeshVertex_Static.glslh"

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/GBufferPrepass_Fragment.glslh"
