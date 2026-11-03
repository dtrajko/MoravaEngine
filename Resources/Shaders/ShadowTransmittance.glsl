// The light that passes through glass (Cast Shadows = Tinted, see EnvMapVulkanGlassShadowMap): a static glass mesh seen from
// a light, into a layer of the transmittance map. Blending multiplies the colors, so two panes in a row dim the light
// twice; alpha keeps the most frosted surface, which softens the shadow's edge (Include/Shadows.glslh).
// Tested against the opaque depth, which it doesn't write: glass behind an opaque mesh doesn't count.
// Drawn with the HazelPBR_Static pipeline's layout, as the cutout variants of ShadowDepth: set 0 (the material entries) and
// the mesh's material set are bound as in the scene pass. The fragment push constants are this shader's own.
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 4) in vec2 a_TexCoord;

layout (push_constant) uniform Transform
{
	mat4 u_LightModelViewProjection;
};

// In the mesh's own space: the light is sent there too (u_Light), and the sign of dot(normal, light) is right there
// whatever the mesh's scale is
layout (location = 0) out vec3 v_Position;
layout (location = 1) out vec3 v_Normal;
layout (location = 2) out vec2 v_TexCoord;

void main()
{
	v_Position = a_Position;
	v_Normal = a_Normal;
	v_TexCoord = a_TexCoord;
	gl_Position = u_LightModelViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

#include "Include/MaterialMaps.glslh"
#include "Include/FrameSet.glslh"
#include "Include/GlassShadow.glslh"

layout (push_constant) uniform Glass
{
	layout (offset = 64) vec4 Light; // in the mesh's space: xyz = towards the light (w = 0), or its position (w = 1)
	float MaterialIndex;             // the material's entry in u_Materials
	float AlbedoMap;                 // 1: the tint is the albedo map
} u_Glass;

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec2 v_TexCoord;

layout (location = 0) out vec4 o_Transmittance;

void main()
{
	// One side of a body counts, the one facing the light: a closed body tints the light once, not on the way in and out
	vec3 N = normalize(v_Normal);
	vec3 L = normalize(u_Glass.Light.xyz - v_Position * u_Glass.Light.w);
	float cosAngle = dot(N, L);
	if (cosAngle <= 0.0)
	{
		discard;
	}

	GPUMaterial material = u_Materials[uint(u_Glass.MaterialIndex + 0.5)];
	vec3 tint = u_Glass.AlbedoMap > 0.5 ? texture(u_AlbedoTexture, v_TexCoord * material.Tiling).rgb : material.AlbedoColor.rgb;
	o_Transmittance = vec4(GlassShadowTransmittance(tint, material.IOR, material.Thickness, material.Roughness, cosAngle), material.Roughness);
}
