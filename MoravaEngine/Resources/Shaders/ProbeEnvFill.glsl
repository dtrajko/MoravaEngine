// Fills the probe volume's atlases from the environment (SceneEnvMapVulkan, see EnvMapVulkanProbes): every probe gets the
// environment's irradiance, as if nothing stood around it, and sees far in every direction. That is what the probes hold
// before they capture the scene, and it makes the probes' sampling testable: lit by these probes, the image must be the
// one lit by the environment directly.
// One thread per texel of the largest atlas (the visibility); each writes the texels the atlases have at its coordinate.
#type compute
#version 450 core

#include "Include/ProbeOctahedral.glslh"

layout (binding = 0) uniform samplerCube u_EnvIrradianceTex;
layout (binding = 1, rgba16f) restrict writeonly uniform image2D o_Irradiance; // per probe: 8 x 8 texels and a border
layout (binding = 2, rg16f) restrict writeonly uniform image2D o_Visibility;   // per probe: 16 x 16 texels and a border
layout (binding = 3, rgba16f) restrict writeonly uniform image2D o_ProbeData;  // per probe: 1 texel

layout (push_constant) uniform Uniforms
{
	vec4 Params; // x = the environment's rotation (degrees around Y), y = the distance every probe sees, z = the irradiance
	             // encoding gamma
} u_Uniforms;

// As in Include/ShadeSurface.glslh: the environment lookups are turned by the environment's rotation
vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix = { vec3(cos(angle), 0.0, sin(angle)),
	                          vec3(0.0, 1.0, 0.0),
	                          vec3(-sin(angle), 0.0, cos(angle)) };
	return rotationMatrix * vec;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);

	ivec2 irradianceSize = imageSize(o_Irradiance);
	if (all(lessThan(texel, irradianceSize)))
	{
		// A border texel repeats an interior texel: it gets that texel's direction
		const int tileSize = ProbeIrradianceTexels + 2;
		ivec2 source = ProbeBorderSource(texel % tileSize, ProbeIrradianceTexels);
		vec3 direction = ProbeTexelDirection(source, ProbeIrradianceTexels);
		vec3 irradiance = texture(u_EnvIrradianceTex, RotateVectorAboutY(u_Uniforms.Params.x, direction)).rgb;
		imageStore(o_Irradiance, texel, vec4(pow(max(irradiance, vec3(0.0)), vec3(1.0 / u_Uniforms.Params.z)), 1.0));
	}

	if (all(lessThan(texel, imageSize(o_Visibility))))
	{
		// The mean distance and the mean squared distance: the same in every direction, so no variance
		float distance = u_Uniforms.Params.y;
		imageStore(o_Visibility, texel, vec4(distance, distance * distance, 0.0, 0.0));
	}

	if (all(lessThan(texel, imageSize(o_ProbeData))))
	{
		imageStore(o_ProbeData, texel, vec4(0.0, 0.0, 0.0, 1.0)); // where the grid puts it, in use
	}
}
