// Shadow map depth pass for the foliage's copies with a cutout material (leaves): ShadowDepth_Cutout.glsl with each copy's
// transform from the instance buffer (binding 1, locations 7 to 10). Drawn with the HazelPBR_Static pipeline's layout, as
// ShadowDepth_Cutout.glsl; the vertex push constants are the light's view-projection alone (the copy's transform follows).
#type vertex
#version 450 core

// The mesh vertex (ModelH2M's Vertex): the position and the texture coordinate
layout(location = 0) in vec3 a_Position;
layout(location = 4) in vec2 a_TexCoord;
layout(location = 7) in mat4 a_InstanceTransform;

layout (push_constant) uniform Transform
{
	mat4 u_LightViewProjection;
};

layout (location = 0) out vec2 v_TexCoord;

void main()
{
	v_TexCoord = a_TexCoord;
	gl_Position = u_LightViewProjection * a_InstanceTransform * vec4(a_Position, 1.0);
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
