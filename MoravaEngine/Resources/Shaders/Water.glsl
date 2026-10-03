// Water surface (SceneEnvMapVulkan, see EnvMapVulkanWater.h): a flat rectangle at the water height, drawn into the HDR
// scene image after the opaque meshes, from a copy of them (their color and depth).
// - waves: two normal map layers, scrolling in different directions at different scales
// - reflection: the environment map (the same prefiltered radiance the PBR shaders use), blended by Schlick Fresnel
//   (water reflects 2% straight down, all of the light at grazing angles)
// - refraction: the scene behind the surface, bent by the waves; light is absorbed on its way through the water, red
//   first (Beer-Lambert), and the water body scatters its own color into the view (more of it the more water)
// - the sun: a GGX highlight on the waves, in the sun's shadow where something shadows the water
// - edges: the surface fades in over the first centimeters of water (no hard line at the shore), with foam there
// - from below: Snell's window (the sky, squeezed into the cone light can leave the water through), total internal
//   reflection outside it, and the water between the camera and the surface (see Include/WaterCommon.glslh)
// Descriptor sets: set 0 is the per-frame set shared with the PBR shaders (declared identically, see
// Include/FrameCamera.glslh and Include/FrameSet.glslh); set 1 is the water's own (Include/WaterCommon.glslh).
#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position; // a unit square in XZ (-0.5..0.5), y = 0

#include "Include/FrameCamera.glslh"

layout (push_constant) uniform Transform
{
	mat4 u_Transform; // the square -> the water rectangle (size, position, height)
};

layout (location = 0) out vec3 v_WorldPosition;

void main()
{
	vec4 worldPosition = u_Transform * vec4(a_Position, 1.0);
	v_WorldPosition = worldPosition.xyz;
	gl_Position = u_ViewProjectionMatrix * worldPosition;
}

#type fragment
#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

layout (location = 0) in vec3 v_WorldPosition;

layout (location = 0) out vec4 color;

#include "Include/FrameSet.glslh"
#include "Include/Shadows.glslh"

#include "Include/WaterCommon.glslh"

// GGX / Trowbridge-Reitz normal distribution (alpha = roughness^2)
float DistributionGGX(float NdotH, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;
	float denom = NdotH * NdotH * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Smith geometry term with Schlick-GGX (Epic's k for analytic lights)
float GeometrySmith(float NdotL, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = r * r / 8.0;
	return (NdotL / (NdotL * (1.0 - k) + k)) * (NdotV / (NdotV * (1.0 - k) + k));
}

// Schlick's Fresnel for water: 2% reflected straight on (index of refraction 1.33)
float FresnelWater(float cosTheta)
{
	const float F0 = 0.02;
	return F0 + (1.0 - F0) * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}

// The distance along the camera's view direction (view space depth) of a depth buffer value
float ViewDepth(float depth)
{
	return -u_DepthParams.y / (depth * u_DepthParams.z - u_DepthParams.x);
}

// The wave normal: two normal map layers, each a slope in X and Z, added (the up component is then normalized back)
vec3 WaveNormal(vec2 worldXZ)
{
	vec3 a = texture(u_WaterNormalMap, worldXZ / u_WaveScale1 + u_WaveOffsets.xy).rbg * 2.0 - 1.0; // x, up, z
	vec3 b = texture(u_WaterNormalMap, worldXZ / u_WaveScale2 + u_WaveOffsets.zw).rbg * 2.0 - 1.0;
	vec2 slope = (a.xz / max(a.y, 0.1) + b.xz / max(b.y, 0.1)) * u_WaveStrength;
	return normalize(vec3(slope.x, 1.0, slope.y));
}

void main()
{
	vec3 N = WaveNormal(v_WorldPosition.xz);
	vec3 V = normalize(u_CameraPosition - v_WorldPosition);
	float roughness = max(u_Roughness, 0.02);
	vec3 up = vec3(0.0, 1.0, 0.0);

	// Seen from below (the camera under the water): the surface faces down, toward the camera
	if (V.y < 0.0)
	{
		// Half the wave slope: near the edge of Snell's window every small tilt switches between the sky and the mirror,
		// and the full slope breaks the window into blotches
		N = -normalize(vec3(N.x * 0.5, N.y, N.z * 0.5));
		vec3 I = -V; // from the camera up to the surface
		vec2 waveSlope = WaveNormal(v_WorldPosition.xz).xz; // the full slope of the waves (N above is calmed for the window)
		vec2 belowUV = gl_FragCoord.xy / vec2(textureSize(u_SceneColor, 0));
		float distanceToSurface = length(u_CameraPosition - v_WorldPosition);

		// The mirror: outside Snell's window the surface reflects all light back down (total internal reflection), so it
		// shows the scene under the water, mirrored: the planar reflection of this frame (the mirrored meshes, already
		// dimmed by the water between them and the surface), the water's own light where it has nothing
		vec3 waterLight = WaterScatteredLight(0.0);
		vec3 mirror = waterLight;
		if (u_PlanarReflection > 0.5)
		{
			float surfaceDepth = ViewDepth(gl_FragCoord.z);
			vec2 reflectionBend = N.xz * u_ReflectionDistortion * 0.03 / max(surfaceDepth * 0.1, 1.0);
			vec2 reflectionUV = clamp(vec2(belowUV.x, 1.0 - belowUV.y) + reflectionBend, vec2(0.001), vec2(0.999));
			vec4 planar = texture(u_ReflectionTexture, reflectionUV);
			mirror = mix(waterLight, planar.rgb, clamp(planar.a, 0.0, 1.0));
		}
		vec3 surfaceColor = mirror;

		// Snell's law: light gets out of the water (index of refraction 1.33) only within about 49 degrees of the vertical
		// (Snell's window). Transparency from Below, up to 0.5, lowers the index toward 1 (no bending): the window widens
		// until only grazing views are mirrored, as any surface is at a grazing angle; from 0.5 to 1 the mirror itself
		// fades (to a tenth of its physical strength), so the surface is see-through up to the horizon.
		float eta = mix(1.33, 1.0, clamp(u_TransparencyFromBelow * 2.0, 0.0, 1.0));
		float mirrorStrength = 1.0 - 0.9 * clamp(u_TransparencyFromBelow * 2.0 - 1.0, 0.0, 1.0);
		vec3 transmitted = refract(I, N, eta);
		if (dot(transmitted, transmitted) > 1e-6)
		{
			// The sky in the refracted direction (the whole sky squeezed into the window), and what stands above the
			// water from the scene copy (bent by the waves) in front of it
			int levels = textureQueryLevels(u_EnvRadianceTex);
			vec3 above = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, transmitted), roughness * levels).rgb;
			vec2 bentUV = clamp(belowUV + N.xz * u_RefractionStrength * 0.04, vec2(0.0), vec2(1.0));
			if (texture(u_SceneDepth, bentUV).r < 1.0)
			{
				above = texture(u_SceneColor, bentUV).rgb;
			}
			// Fresnel on the way out of the denser medium: Schlick with the angle on the air side, rising to 1 at the
			// edge of the window
			float F = FresnelWater(dot(transmitted, -N)) * mirrorStrength;
			surfaceColor = mix(above, mirror, F);
		}
		// The sun glittering through the surface: where the sun shows at this transparency (refracted with the window's
		// index), each wave facet bends the view by its own slope, so the sun breaks into glints that move with the waves.
		// A GGX lobe a few degrees wide (it integrates to the sun's irradiance); shadowed where something above the water
		// hides the sun; dimmed by the light the surface reflects back at that angle.
		if (u_Sun.Intensity > 0.0 && u_Sun.Direction.y > 0.0)
		{
			vec3 flatTransmitted = refract(I, vec3(0.0, -1.0, 0.0), eta);
			if (dot(flatTransmitted, flatTransmitted) > 1e-6)
			{
				vec3 glintDirection = normalize(flatTransmitted + vec3(waveSlope.x, 0.0, waveSlope.y) * (WaterIndexOfRefraction - 1.0) * 1.5);
				float leaving = 1.0 - FresnelWater(flatTransmitted.y) * mirrorStrength;
				float alignment = max(dot(glintDirection, u_Sun.Direction), 0.0);
				surfaceColor += u_Sun.Color * u_Sun.Intensity * SunShadow(v_WorldPosition, up) * DistributionGGX(alignment, max(roughness, 0.2)) * leaving;
			}
		}

		// The water between the camera and the surface
		color = vec4(ApplyWaterVolume(surfaceColor, u_CameraPosition, I, distanceToSurface), 1.0);
		if (u_ShadowDebug.x > 0.5)
		{
			color = vec4(vec3(0.2), 1.0); // Shadows Only: not lit by the lights
		}
		return;
	}

	float NdotV = max(dot(N, V), Epsilon);

	// How much water the view crosses before it reaches the scene behind the surface: the scene's view depth here minus
	// the surface's, along the view ray (both are view space depths, so their ratio scales the distance to the surface)
	vec2 screenSize = vec2(textureSize(u_SceneColor, 0));
	vec2 screenUV = gl_FragCoord.xy / screenSize;
	float surfaceDepth = ViewDepth(gl_FragCoord.z);
	float surfaceDistance = length(u_CameraPosition - v_WorldPosition);
	float pathPerDepth = surfaceDistance / max(surfaceDepth, Epsilon);
	float sceneDepth = ViewDepth(texture(u_SceneDepth, screenUV).r);
	float waterPath = max(sceneDepth - surfaceDepth, 0.0) * pathPerDepth;

	// Refraction: the waves bend the view, more where there is more water (no shifted image at the shore), and less with
	// distance, like the waves themselves look smaller. A bent sample that lands on something in front of the water (a
	// pillar standing in it) would smear it into the water: there the unbent view is used.
	vec2 bend = N.xz * u_RefractionStrength * 0.04 * clamp(waterPath, 0.0, 1.0) / max(surfaceDepth * 0.1, 1.0);
	vec2 refractedUV = clamp(screenUV + bend, vec2(0.0), vec2(1.0));
	float refractedDepth = ViewDepth(texture(u_SceneDepth, refractedUV).r);
	if (refractedDepth < surfaceDepth)
	{
		refractedUV = screenUV;
		refractedDepth = sceneDepth;
	}
	vec3 refracted = texture(u_SceneColor, refractedUV).rgb;
	float refractedPath = max(refractedDepth - surfaceDepth, 0.0) * pathPerDepth;

	// The sun: shadowed like any surface (with the flat surface normal: the waves don't move the shadow)
	vec3 sunRadiance = vec3(0.0);
	if (u_Sun.Intensity > 0.0 && u_Sun.Direction.y > 0.0)
	{
		sunRadiance = u_Sun.Color * u_Sun.Intensity * SunShadow(v_WorldPosition, up);
	}

	// Into the water: the scene behind the surface loses light along its path (Beer-Lambert), and the water body scatters
	// its own color (lit by the sky and the sun) into the view in its place
	vec3 transmittance = exp(-u_Absorption * refractedPath);
	vec3 skyIrradiance = texture(u_EnvIrradianceTex, RotateVectorAboutY(u_EnvMapRotation, up)).rgb;
	vec3 incoming = skyIrradiance + sunRadiance * max(u_Sun.Direction.y, 0.0);
	vec3 underwater = refracted * transmittance + u_ScatterColor * incoming * (1.0 - transmittance);

	// Reflection: the environment in the mirrored view direction. The waves can tilt it below the horizon, and the water
	// does not reflect what is under its own surface, so it is kept at the horizon.
	vec3 R = reflect(-V, N);
	R.y = abs(R.y);
	int radianceLevels = textureQueryLevels(u_EnvRadianceTex);
	vec3 reflection = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, R), roughness * radianceLevels).rgb;

	// The planar reflection over it: the mirrored scene at this point of the screen (the image is flipped vertically),
	// bent by the waves like the refraction; its alpha says where it has something (its edges blend into the sky)
	if (u_PlanarReflection > 0.5)
	{
		vec2 reflectionBend = N.xz * u_ReflectionDistortion * 0.03 / max(surfaceDepth * 0.1, 1.0);
		vec2 reflectionUV = clamp(vec2(screenUV.x, 1.0 - screenUV.y) + reflectionBend, vec2(0.001), vec2(0.999));
		vec4 planar = texture(u_ReflectionTexture, reflectionUV);
		reflection = mix(reflection, planar.rgb, clamp(planar.a, 0.0, 1.0));
	}
	reflection *= u_ReflectionStrength;
	float F = FresnelWater(NdotV);

	// The sun's highlight on the waves (GGX)
	vec3 specular = vec3(0.0);
	float NdotL = dot(N, u_Sun.Direction);
	if (NdotL > 0.0 && dot(sunRadiance, sunRadiance) > 0.0)
	{
		vec3 H = normalize(u_Sun.Direction + V);
		float D = DistributionGGX(max(dot(N, H), 0.0), roughness);
		float G = GeometrySmith(NdotL, NdotV, roughness);
		float FH = FresnelWater(dot(H, V));
		specular = sunRadiance * (D * G * FH / max(4.0 * NdotL * NdotV, Epsilon)) * NdotL;
	}

	vec3 surface = F * reflection + (1.0 - F) * underwater + specular;

	// The depth of water under the surface (vertical), for the foam and the edges
	float verticalDepth = waterPath * max(V.y, 0.0);

	// Foam where the water is shallow (the shore, around objects in the water): the wave pattern decides where, so it
	// breaks up and moves with the waves; lit like a white, rough surface
	if (u_FoamAmount > 0.0)
	{
		float shallow = 1.0 - clamp(verticalDepth / u_FoamWidth, 0.0, 1.0);
		float pattern = texture(u_WaterNormalMap, v_WorldPosition.xz / (u_WaveScale2 * 0.5) + u_WaveOffsets.zw * 2.0).r;
		float foam = smoothstep(1.0 - shallow, 1.0 - shallow + 0.15, pattern) * shallow * u_FoamAmount;
		surface = mix(surface, vec3(0.9) * incoming, clamp(foam, 0.0, 1.0));
	}

	// Edges: over the first EdgeSoftness of water the surface fades into the scene behind it (no hard line at the shore)
	float edge = clamp(verticalDepth / u_EdgeSoftness, 0.0, 1.0);
	color = vec4(mix(texture(u_SceneColor, screenUV).rgb, surface, edge), 1.0);

	// Shadows Only (Lights panel): the sun's shadow on the water, gray for the other lights (they don't light the water yet)
	if (u_ShadowDebug.x > 0.5)
	{
		color = vec4(vec3(u_ShadowDebug.x < 1.5 ? SunShadow(v_WorldPosition, up) : 0.2), 1.0);
	}
}
