// The path tracer (SceneEnvMapVulkan, see EnvMapVulkanPathTracer and Phase H.4 of
// docs/rendering/SSAO_GI_RayTracing_Guide.html): the whole image by rays, as a reference to compare the renderer with.
// Nothing here comes from the rasterizer, the probes, the ambient occlusion or the reflection image: a pixel's light is
// found by following its path through the scene, bounce after bounce, as light really travels (backwards: from the
// camera). One path is one random sample, far too noisy to look at; the image is the average of all the paths traced
// since the camera or the scene last changed, and gets cleaner with every frame it stands still.
//
// A path (TracePath):
//   - a ray from the camera through a random point of the pixel (so edges come out smooth);
//   - where it hits a surface: what the surface emits, and the light of the sun, the point and the spot lights arriving
//     there (a shadow ray each), through the same BRDF as the mesh shaders' (EvaluateBRDF in Include/PBRCommon.glslh);
//   - then one direction to go on in, chosen at random the way the surface scatters light: as a reflection (mirrored at
//     a facet drawn from the roughness) or as matte light (any direction over the surface, more often near its normal).
//     What the next ray finds there is the indirect light: bounced light, reflections and the occlusion of both, all in
//     one. The path carries how much of that light the surfaces so far pass on (its throughput);
//   - glass lets the ray through (tinted), bent by its index of refraction where it is solid, or reflects it (more at
//     grazing angles): so glass shows what is really behind it, other glass included;
//   - a ray that hits nothing gets the environment's light, and the path ends. It also ends after Max Bounces, or
//     earlier by chance when it carries little light any more (the paths that go on count more: the average stays right).
// Not followed: light through glass to a surface (a shadow ray stops at glass that casts a shadow and ignores glass
// that doesn't, as the shadow maps do), and the water (it isn't in the acceleration structures).
// Sets: 0 the per-frame set's contents (lights, environment), 1 the image the samples are averaged in, 3 the ray tracing
// set.
#type compute
#version 460 core

#include "Include/RayTracing.glslh"

// Must match EnvMapVulkanPathTracer::Push
layout (push_constant) uniform PathTracePush
{
	mat4 InverseViewProjection; // offset 0:  clip space to world space, of the camera the scene is drawn with
	ivec4 Counts;               // offset 64: x = the samples the image holds (0: start over), y = samples to add now,
	                            //            z = the max bounces
	vec4 Params;                // offset 80: x = 1: the sun casts shadows, y = a sample's brightest value after its first
	                            //            bounce (0: no limit), z = the skybox's level of the environment map,
	                            //            w = the angle a pixel covers
} u_Path;

#define RAY_TRACED_SUN_SHADOWS (u_Path.Params.x > 0.5)
#include "Include/ShadeSurfaceRayTraced.glslh"

// The average of the samples so far (linear light, as the scene pass's image)
layout (set = 1, binding = 0, rgba32f) uniform image2D u_Accumulation;

const float PI = 3.14159265359;
const float MissDistance = 10000.0;
const int MaxGlassPasses = 12;        // glass surfaces a path may cross or be reflected by (they aren't bounces)
const int RouletteStart = 3;          // from this bounce on a path may end by chance
const float DiffuseSkyLevel = 0.3;    // of the environment map's levels: what a matte bounce reads (see the miss)

uint Hash(uint value)
{
	uint state = value * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float NextRandom(inout uint state)
{
	state = Hash(state);
	return float(state) * (1.0 / 4294967296.0);
}

float Brightness(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// The values a surface scatters light by
struct PathSurface
{
	vec3 Albedo;
	float Metalness;
	float Roughness;
	vec3 F0; // what it reflects seen head-on
};

vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// How much of the light a smooth boundary between two clear materials reflects (the rest passes through): Fresnel's
// equations for light that isn't polarized. cosine: of the angle between the ray and the boundary's normal, on the side
// the ray comes from; eta: the index of refraction behind the boundary divided by the one in front of it.
// 1: no way through at this angle (total reflection, from the denser side).
float FresnelDielectric(float cosine, float eta)
{
	float sineSquaredBehind = (1.0 - cosine * cosine) / (eta * eta);
	if (sineSquaredBehind >= 1.0)
	{
		return 1.0;
	}
	float cosineBehind = sqrt(1.0 - sineSquaredBehind);
	float across = (cosine - eta * cosineBehind) / (cosine + eta * cosineBehind);
	float along = (eta * cosine - cosineBehind) / (eta * cosine + cosineBehind);
	return 0.5 * (across * across + along * along);
}

// The light of one light the surface sends towards V: EvaluateBRDF of Include/PBRCommon.glslh (the same terms, so a
// surface lit by a light alone looks as the mesh shaders draw it)
vec3 EvaluateLight(PathSurface surface, vec3 N, vec3 V, vec3 L, vec3 radiance)
{
	float cosLi = dot(N, L);
	if (cosLi <= 0.0)
	{
		return vec3(0.0);
	}
	float NdotV = max(dot(N, V), 0.0);
	vec3 Lh = normalize(L + V);
	float cosLh = max(dot(N, Lh), 0.0);

	vec3 F = FresnelSchlick(surface.F0, max(dot(Lh, V), 0.0));
	float alpha = surface.Roughness * surface.Roughness;
	float alphaSq = alpha * alpha;
	float denominator = cosLh * cosLh * (alphaSq - 1.0) + 1.0;
	float D = alphaSq / (PI * denominator * denominator);
	float r = surface.Roughness + 1.0;
	float k = r * r / 8.0;
	float G = (cosLi / (cosLi * (1.0 - k) + k)) * (NdotV / (NdotV * (1.0 - k) + k));

	vec3 kd = (1.0 - F) * (1.0 - surface.Metalness);
	vec3 specular = F * D * G / max(Epsilon, 4.0 * cosLi * NdotV);
	return (kd * surface.Albedo + specular) * radiance * cosLi;
}

// The sun's, the point lights' and the spot lights' light a surface point sends towards V (Include/ShadeSurface.glslh:
// falloff, cones and, by shadow rays, shadows)
vec3 DirectLight(PathSurface surface, vec3 P, vec3 N, vec3 geometryNormal, vec3 V)
{
	vec3 result = vec3(0.0);
	vec3 L, radiance;
	if (SunLightAt(P, N, geometryNormal, L, radiance))
	{
		result += EvaluateLight(surface, N, V, L, radiance);
	}
	int pointLightCount = min(u_PointLightCount, MaxPointLights);
	for (int i = 0; i < pointLightCount; i++)
	{
		if (PointLightAt(i, P, N, geometryNormal, L, radiance))
		{
			result += EvaluateLight(surface, N, V, L, radiance);
		}
	}
	int spotLightCount = min(u_SpotLightCount, MaxSpotLights);
	for (int i = 0; i < spotLightCount; i++)
	{
		if (SpotLightAt(i, P, N, geometryNormal, L, radiance))
		{
			result += EvaluateLight(surface, N, V, L, radiance);
		}
	}
	return result;
}

// A frame around a normal: the columns turn a direction given against the normal (z) into world space
mat3 FrameAround(vec3 N)
{
	vec3 tangent = normalize(abs(N.y) < 0.99 ? cross(N, vec3(0.0, 1.0, 0.0)) : cross(N, vec3(1.0, 0.0, 0.0)));
	return mat3(tangent, cross(N, tangent), N);
}

// A facet normal of a GGX surface (alpha: its roughness squared) among those visible from V, in the frame where the
// surface normal is z (Heitz 2018, "Sampling the GGX Distribution of Visible Normals")
vec3 SampleVisibleFacet(vec3 V, float alpha, float u1, float u2)
{
	vec3 stretched = normalize(vec3(alpha * V.x, alpha * V.y, V.z));
	float lengthSquared = stretched.x * stretched.x + stretched.y * stretched.y;
	vec3 t1 = lengthSquared > 0.0 ? vec3(-stretched.y, stretched.x, 0.0) / sqrt(lengthSquared) : vec3(1.0, 0.0, 0.0);
	vec3 t2 = cross(stretched, t1);
	float r = sqrt(u1);
	float phi = 2.0 * PI * u2;
	float p1 = r * cos(phi);
	float p2 = r * sin(phi);
	float s = 0.5 * (1.0 + stretched.z);
	p2 = (1.0 - s) * sqrt(max(1.0 - p1 * p1, 0.0)) + s * p2;
	vec3 facet = p1 * t1 + p2 * t2 + sqrt(max(1.0 - p1 * p1 - p2 * p2, 0.0)) * stretched;
	return normalize(vec3(alpha * facet.x, alpha * facet.y, max(facet.z, 0.0)));
}

// A facet of the surface with normal N as seen from V, in world space
vec3 SampleFacet(vec3 N, vec3 V, float alpha, inout uint random)
{
	mat3 frame = FrameAround(N);
	float u1 = NextRandom(random);
	float u2 = NextRandom(random);
	return frame * SampleVisibleFacet(V * frame, alpha, u1, u2);
}

// How much of a GGX surface's facets are not hidden by other facets, seen at the angle cosTheta (Smith)
float FacetVisibility(float cosTheta, float alpha)
{
	float alphaSq = alpha * alpha;
	return 2.0 * cosTheta / (cosTheta + sqrt(alphaSq + (1.0 - alphaSq) * cosTheta * cosTheta));
}

// The light arriving at the camera along one random path that starts with this ray
vec3 TracePath(vec3 origin, vec3 direction, inout uint random)
{
	vec3 radiance = vec3(0.0);
	vec3 throughput = vec3(1.0); // how much of the light found from here on reaches the camera
	int maxBounces = u_Path.Counts.z;
	float brightest = u_Path.Params.y;
	int bounce = 0;            // the times the path was scattered by an opaque surface
	int passes = 0;            // the glass surfaces it met
	bool afterDiffuse = false; // it was scattered as matte light at least once
	float width = 0.0;         // how wide the path is where it is now (world units), for the maps' level
	float spread = u_Path.Params.w; // how fast it widens (an angle): a pixel's, more after every scattering

	while (true)
	{
		RayHit hit = TraceRay(origin, direction, 0.0, MissDistance, RayMaskAll);
		if (!hit.Hit)
		{
			// The environment. Seen directly (or through glass): as the skybox shows it. By a matte bounce: a blurred
			// level. The bounce's rays cover the whole sky evenly, so its average is the same, and a small very bright
			// spot (the sun's disc in the map) is found a little by every path instead of fully by one in thousands.
			float levels = float(textureQueryLevels(u_EnvRadianceTex));
			float lod = afterDiffuse ? DiffuseSkyLevel * levels : (bounce == 0 ? u_Path.Params.z : 0.0);
			vec3 sky = throughput * textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, direction), lod).rgb;
			radiance += bounce > 0 && brightest > 0.0 ? min(sky, vec3(brightest)) : sky;
			break;
		}

		width += hit.T * spread;
		RaySurface raySurface = GetRaySurface(hit);
		RayMaterial material = GetRayMaterial(raySurface, width);
		vec3 P = raySurface.Position;
		vec3 V = -direction;
		// The side the ray arrives on: the triangle's own normal and the shading normal, both towards it
		bool frontSide = dot(raySurface.GeometricNormal, V) > 0.0;
		vec3 geometryNormal = frontSide ? raySurface.GeometricNormal : -raySurface.GeometricNormal;
		vec3 N = frontSide ? material.Normal : -material.Normal;
		float NdotV = dot(N, V);
		if (NdotV < 0.01)
		{
			N = normalize(N + V * (0.01 - NdotV)); // a normal map can tilt the normal away from the viewer
			NdotV = dot(N, V);
		}
		float alpha = material.Roughness * material.Roughness;

		vec3 found = throughput * material.Emissive;
		vec3 next;
		bool through = false; // the path goes on behind the surface

		if ((material.Flags & GPUMaterialFlagGlass) != 0u)
		{
			// Glass: nothing matte. It reflects (the lights too: their highlights) or lets the ray through.
			float ior = max(material.IOR, 1.0);
			float f0 = (ior - 1.0) / (ior + 1.0);
			f0 *= f0;
			PathSurface glass;
			glass.Albedo = vec3(0.0);
			glass.Metalness = 0.0;
			glass.Roughness = material.Roughness;
			glass.F0 = vec3(f0);
			if (frontSide)
			{
				found += throughput * DirectLight(glass, P, N, geometryNormal, V);
			}
			radiance += bounce > 0 && brightest > 0.0 ? min(found, vec3(brightest)) : found;
			if (++passes > MaxGlassPasses)
			{
				break;
			}

			// The facet the ray meets (frosted glass: tilted by the roughness), and how much it reflects at that angle
			vec3 facet = SampleFacet(N, V, alpha, random);
			float cosine = clamp(dot(V, facet), 0.0, 1.0);
			bool solid = (material.Flags & GPUMaterialFlagGlassSolid) != 0u;
			vec3 refracted;
			float reflectance;
			if (solid)
			{
				// A solid body: the ray is bent on the way in, and again on the way out. From the inside there is no
				// way out at a flat angle: all of it is reflected back in.
				float eta = frontSide ? ior : 1.0 / ior;
				reflectance = FresnelDielectric(cosine, eta);
				refracted = refract(direction, facet, 1.0 / eta);
			}
			else
			{
				// Thin glass (a pane, a shell): two boundaries close together. The ray leaves through the far one in its
				// old direction; frosted, as widely scattered as its reflection (the reflection's direction, mirrored to
				// the other side). What the two reflect together, with the light going back and forth between them.
				float single = FresnelDielectric(cosine, ior);
				reflectance = 2.0 * single / (1.0 + single);
				vec3 mirrored = reflect(direction, facet);
				refracted = mirrored - 2.0 * dot(mirrored, N) * N;
			}
			if (NextRandom(random) < reflectance || dot(refracted, refracted) <= 0.0)
			{
				next = reflect(direction, facet);
			}
			else
			{
				next = normalize(refracted);
				through = true;
				if (frontSide)
				{
					throughput *= material.Albedo; // the glass's tint, once per body: on the way in
				}
			}
			spread += alpha;
		}
		else
		{
			PathSurface surface;
			surface.Albedo = material.Albedo;
			surface.Metalness = material.Metalness;
			surface.Roughness = material.Roughness;
			surface.F0 = mix(vec3(0.04), material.Albedo, material.Metalness);
			found += throughput * DirectLight(surface, P, N, geometryNormal, V);
			radiance += bounce > 0 && brightest > 0.0 ? min(found, vec3(brightest)) : found;
			if (bounce >= maxBounces)
			{
				break;
			}
			bounce++;

			// One of the two ways the surface scatters light, each as often as it matters at this angle: the reflection
			// (Fresnel), or the matte light (what the reflection leaves, of a non-metal). The path's light is divided
			// by how often its way is chosen, so both keep their share on average.
			vec3 F = FresnelSchlickRoughness(surface.F0, NdotV, surface.Roughness);
			vec3 matte = surface.Albedo * (1.0 - surface.Metalness) * (1.0 - F);
			float reflectionShare = Brightness(F);
			float matteShare = Brightness(matte);
			float chooseReflection = matteShare > 0.0 ? clamp(reflectionShare / (reflectionShare + matteShare), 0.1, 0.9) : 1.0;
			if (NextRandom(random) < chooseReflection)
			{
				// Mirrored at a facet drawn from the roughness; what is left to weigh: the facet's Fresnel, and how much
				// of the reflected light other facets hide
				vec3 facet = SampleFacet(N, V, alpha, random);
				next = reflect(direction, facet);
				float NdotL = dot(N, next);
				if (NdotL <= 0.0)
				{
					break;
				}
				throughput *= FresnelSchlick(surface.F0, max(dot(V, facet), 0.0)) * FacetVisibility(NdotL, alpha) / chooseReflection;
				spread += alpha;
			}
			else
			{
				// Any direction over the surface, more often near the normal (by the cosine: as much as light from there
				// counts for a matte surface, so nothing is left to weigh but its color)
				float u1 = NextRandom(random);
				float u2 = NextRandom(random);
				float radius = sqrt(u1);
				float angle = 2.0 * PI * u2;
				next = FrameAround(N) * vec3(radius * cos(angle), radius * sin(angle), sqrt(max(1.0 - u1, 0.0)));
				throughput *= matte / (1.0 - chooseReflection);
				afterDiffuse = true;
				spread = max(spread, 0.5);
			}

			// A path that carries little light ends by chance; the ones that go on count for those too
			if (bounce >= RouletteStart)
			{
				float survive = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 1.0);
				if (NextRandom(random) > survive)
				{
					break;
				}
				throughput /= survive;
			}
		}

		// The next ray starts on the side it leaves on; a direction that points into the surface it should leave from
		// (a bent shading normal, a steep facet) is mirrored back to that side
		vec3 leaveNormal = through ? -geometryNormal : geometryNormal;
		float outOfSurface = dot(next, leaveNormal);
		if (outOfSurface < 0.0)
		{
			next -= 2.0 * outOfSurface * leaveNormal;
		}
		origin = RayOriginOffSurface(P, leaveNormal);
		direction = next;
		if (dot(throughput, throughput) <= 0.0)
		{
			break;
		}
	}
	return radiance;
}

layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 size = imageSize(u_Accumulation);
	if (any(greaterThanEqual(pixel, size)))
	{
		return;
	}
	int have = u_Path.Counts.x;
	int count = u_Path.Counts.y;

	vec3 sum = vec3(0.0);
	for (int i = 0; i < count; i++)
	{
		// Other random numbers for every pixel and every sample
		uint random = Hash(uint(pixel.x) + Hash(uint(pixel.y) + Hash(uint(have + i))));
		// The ray through a random point of the pixel: from where that is on the near plane towards where it is on the
		// far plane, so it covers what the rasterizer draws for the pixel
		vec2 inPixel = vec2(NextRandom(random), NextRandom(random));
		vec2 ndc = (vec2(pixel) + inPixel) / vec2(size) * 2.0 - 1.0;
		vec4 nearPoint = u_Path.InverseViewProjection * vec4(ndc, 0.0, 1.0);
		vec4 farPoint = u_Path.InverseViewProjection * vec4(ndc, 1.0, 1.0);
		vec3 origin = nearPoint.xyz / nearPoint.w;
		vec3 direction = normalize(farPoint.xyz / farPoint.w - origin);

		vec3 sampled = TracePath(origin, direction, random);
		if (!any(isnan(sampled)) && !any(isinf(sampled)))
		{
			sum += sampled;
		}
	}

	// The average of all samples: the image's, and the new ones
	vec3 average = have > 0 ? imageLoad(u_Accumulation, pixel).rgb : vec3(0.0);
	average = (average * float(have) + sum) / float(have + count);
	imageStore(u_Accumulation, pixel, vec4(average, 1.0));
}
