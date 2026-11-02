// The ray-traced debug views (SceneEnvMapVulkan, see EnvMapVulkanRayTracing and Phase G of
// docs/rendering/SSAO_GI_RayTracing_Guide.html): one ray per pixel from the camera, and what it hit as a display color.
// The check of everything a ray-traced effect depends on: the acceleration structures (the meshes are where they are
// drawn, the skinned ones in their pose), the vertices and indices read at the hit, the material buffer and the array
// of textures. The Normals view must show what the G-buffer's Normals view shows.
// The viewport composite shows the image as it is (ViewportComposite.glsl).
#type compute
#version 460 core

#include "Include/RayTracing.glslh"

layout (set = 0, binding = 0, rgba8) restrict writeonly uniform image2D o_View;

// Must match EnvMapVulkanRayTracing::ViewPush
layout (push_constant) uniform RayTracingViewPush
{
	mat4 InverseViewProjection; // offset 0:  clip space to world space, of the camera the scene is drawn with
	ivec4 Params;               // offset 64: x = the view: 0 normals, 1 albedo, 2 instances; y = the ray mask
} u_View;

// A color per number, far apart for numbers close together
vec3 IndexColor(uint index)
{
	float hue = fract(float(index) * 0.61803398875);
	vec3 color = clamp(abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
	return mix(vec3(1.0), color, 0.75);
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(o_View);
	if (any(greaterThanEqual(pixel, size)))
	{
		return;
	}

	// The pixel's ray: from where its center is on the near plane to where it is on the far plane, so it covers exactly
	// what the rasterizer draws for the pixel
	vec2 ndc = (vec2(pixel) + 0.5) / vec2(size) * 2.0 - 1.0;
	vec4 nearPoint = u_View.InverseViewProjection * vec4(ndc, 0.0, 1.0);
	vec4 farPoint = u_View.InverseViewProjection * vec4(ndc, 1.0, 1.0);
	vec3 origin = nearPoint.xyz / nearPoint.w;
	vec3 toFar = farPoint.xyz / farPoint.w - origin;
	float rayLength = length(toFar);
	vec3 direction = toFar / rayLength;

	// How far apart two neighboring pixels' rays are after one unit of the way: a pixel's width at a distance
	vec4 nextNearPoint = u_View.InverseViewProjection * vec4(ndc + vec2(2.0 / float(size.x), 0.0), 0.0, 1.0);
	vec4 nextFarPoint = u_View.InverseViewProjection * vec4(ndc + vec2(2.0 / float(size.x), 0.0), 1.0, 1.0);
	vec3 nextOrigin = nextNearPoint.xyz / nextNearPoint.w;
	float pixelSpread = length(normalize(nextFarPoint.xyz / nextFarPoint.w - nextOrigin) - direction);
	float originDistance = length(origin - nextOrigin) / max(pixelSpread, 1e-6); // from the eye to the near plane

	RayHit hit = TraceRay(origin, direction, 0.0, rayLength, uint(u_View.Params.y));
	vec3 color = vec3(0.0); // no surface: the sky
	if (hit.Hit)
	{
		RaySurface surface = GetRaySurface(hit);
		// The maps at the level a pixel covers there. On a surface seen at a slant a pixel covers a long strip: the
		// rasterizer's sampler averages along it (anisotropic filtering), one sample here can only pick a level between
		// the strip's width and its length. So the views match the rasterized ones up to this filtering.
		float slant = sqrt(max(abs(dot(surface.GeometricNormal, direction)), 0.05));
		RayMaterial material = GetRayMaterial(surface, (hit.T + originDistance) * pixelSpread / slant);
		if (u_View.Params.x == 0)
		{
			// As the G-buffer's Normals view: x red, y green, z blue, from -1..1 to 0..1
			color = material.Normal * 0.5 + 0.5;
		}
		else if (u_View.Params.x == 1)
		{
			color = pow(material.Albedo, vec3(1.0 / 2.2)); // linear to display
		}
		else
		{
			// A color per mesh instance, darker where the surface turns away from the camera (so shapes can be read)
			float facing = abs(dot(normalize(surface.Normal), direction));
			color = IndexColor(uint(hit.Instance)) * (0.35 + 0.65 * facing);
		}
	}
	imageStore(o_View, pixel, vec4(color, 1.0));
}
