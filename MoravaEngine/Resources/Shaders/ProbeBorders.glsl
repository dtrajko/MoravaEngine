// Probe update, step 5 (SceneEnvMapVulkan, see EnvMapVulkanProbes and Include/ProbeUpdate.glslh): after the blends,
// the border texels of the batch's tiles in both atlases. A border texel repeats the interior texel across the
// octahedron's seam (ProbeBorderSource in Include/ProbeOctahedral.glslh), so linear filtering works up to a tile's edge.
// A group per probe of the batch. Its threads walk the four sides of the larger tile (the visibility's, 18 texels a
// side): y = the side, x = the texel along it. The irradiance tile (10 a side) uses the first 10 of each side.
// Reads interior texels and writes border texels only, so no thread reads what another writes.
#type compute
#version 450 core

#include "Include/ProbeUpdate.glslh"

layout (binding = 0, rgba16f) restrict uniform image2D u_Irradiance;
layout (binding = 1, rg16f) restrict uniform image2D u_Visibility;

// The texel of a tile at position along one of its sides: 0 top row, 1 bottom row, 2 left column, 3 right column
ivec2 SideTexel(int side, int along, int tileSize)
{
	if (side == 0) return ivec2(along, 0);
	if (side == 1) return ivec2(along, tileSize - 1);
	if (side == 2) return ivec2(0, along);
	return ivec2(tileSize - 1, along);
}

layout (local_size_x = 18, local_size_y = 4, local_size_z = 1) in;
void main()
{
	ivec3 probe = BatchProbe(int(gl_WorkGroupID.x));
	int along = int(gl_LocalInvocationID.x);
	int side = int(gl_LocalInvocationID.y);

	// The corners belong to the rows (sides 0 and 1): the columns leave them out, so every border texel is written once
	const int irradianceTile = ProbeIrradianceTexels + 2;
	if (along < irradianceTile && (side < 2 || (along > 0 && along < irradianceTile - 1)))
	{
		ivec2 origin = ProbeTileOrigin(probe, ProbeIrradianceTexels);
		ivec2 inTile = SideTexel(side, along, irradianceTile);
		imageStore(u_Irradiance, origin + inTile, imageLoad(u_Irradiance, origin + ProbeBorderSource(inTile, ProbeIrradianceTexels)));
	}

	const int visibilityTile = ProbeVisibilityTexels + 2;
	if (side < 2 || (along > 0 && along < visibilityTile - 1))
	{
		ivec2 origin = ProbeTileOrigin(probe, ProbeVisibilityTexels);
		ivec2 inTile = SideTexel(side, along, visibilityTile);
		imageStore(u_Visibility, origin + inTile, imageLoad(u_Visibility, origin + ProbeBorderSource(inTile, ProbeVisibilityTexels)));
	}
}
