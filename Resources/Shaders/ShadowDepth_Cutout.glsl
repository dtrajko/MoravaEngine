// Shadow map depth pass for static meshes with a cutout material (EnvMapVulkanMaterial::Surface::Cutout): ShadowDepth.glsl
// with the pixels where the albedo map's alpha is below the cutoff discarded, so the holes let the light through.
// Drawn with the HazelPBR_Static pipeline's layout (see DrawShadowCasters): the per-frame set (the material entries) and
// the mesh's material set are bound as for the scene pass, and so are the material values (the fragment push constants).
// The vertex push constants are the light's view-projection times the mesh's transform.
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's Vertex): the position and the texture coordinate
layout(location = 0) in vec3 a_Position;
layout(location = 4) in vec2 a_TexCoord;

layout (push_constant) uniform Transform
{
	mat4 u_LightModelViewProjection;
};

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_LightModelViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

#define MATERIAL_CUTOUT
#include "Include/MeshMaterial.glslh"

layout (location = 0) in vec2 v_TexCoord;

void main()
{
	MaterialCutAway(MaterialTexCoord(v_TexCoord));
}
