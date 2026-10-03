// Water surface (SceneEnvMapVulkan, see EnvMapVulkanWater.h): a flat rectangle at the water height, drawn into the HDR
// scene image after the opaque meshes, from a copy of them (their color and depth).
// - waves: two normal map layers, scrolling in different directions at different scales
// - reflection: the environment map (the same prefiltered radiance the PBR shaders use), blended by Schlick Fresnel
//   (water reflects 2% straight down, all of the light at grazing angles)
// - refraction: the scene behind the surface, bent by the waves; light is absorbed on its way through the water, red
//   first (Beer-Lambert), and the water body scatters its own color into the view (more of it the more water)
// - the sun: a GGX highlight on the waves, in the sun's shadow where something shadows the water
// - edges: the surface fades in over the first centimeters of water (no hard line at the shore), with foam there
// Descriptor sets: set 0 is the per-frame set shared with the PBR shaders (declared identically, see
// Include/FrameCamera.glslh and Include/FrameSet.glslh); set 1 is the water's own (normal map, settings, scene copy).
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

// Set 1: the water's own resources
layout (set = 1, binding = 0) uniform sampler2D u_WaterNormalMap; // x in R, up in B, z in G
layout (std140, set = 1, binding = 1) uniform WaterSettings
{
	vec4 u_WaveOffsets;     // offset 0: xy = normal map layer 1 offset, zw = layer 2 (in normal map tiles, scrolled every frame)
	vec3 u_ScatterColor;    // offset 16: the color of the light the water body sends back up (darker = deeper, clearer water)
	float u_Roughness;      // offset 28: of the surface (the sun highlight and the blur of the reflection)
	float u_WaveScale1;     // offset 32: world size of one tile of normal map layer 1
	float u_WaveScale2;     // offset 36: of layer 2
	float u_WaveStrength;   // offset 40: steepness of the waves (scales the normals' slope)
	float u_ReflectionStrength; // offset 44: multiplies the environment reflection (1 = physically based)
	vec4 u_DepthParams;     // offset 48: depth -> view space z: z = y / (depth * z - x); w = the water height
	vec3 u_Absorption;      // offset 64: per meter, per color
	float u_RefractionStrength; // offset 76: how much the waves bend the view into the water
	float u_EdgeSoftness;   // offset 80: meters of water over which the surface fades in
	float u_FoamAmount;     // offset 84: 0 = no foam
	float u_FoamWidth;      // offset 88: meters of water that get foam
	float u_SettingsPadding;
};
// The opaque scene before the water (copied from the scene framebuffer every frame)
layout (set = 1, binding = 2) uniform sampler2D u_SceneColor;
layout (set = 1, binding = 3) uniform sampler2D u_SceneDepth;

vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix = { vec3(cos(angle), 0.0, sin(angle)),
	                          vec3(0.0, 1.0, 0.0),
	                          vec3(-sin(angle), 0.0, cos(angle)) };
	return rotationMatrix * vec;
}

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
	bool fromBelow = V.y < 0.0;
	// Seen from below, the surface faces down (the underwater look comes in a later phase)
	if (fromBelow)
	{
		N = -N;
	}
	float NdotV = max(dot(N, V), Epsilon);
	float roughness = max(u_Roughness, 0.02);
	vec3 up = vec3(0.0, 1.0, 0.0);

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
	// Seen from below: the scene above the surface, no water between (the underwater fog is a later phase)
	float refractedPath = fromBelow ? 0.0 : max(refractedDepth - surfaceDepth, 0.0) * pathPerDepth;

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
	vec3 reflection = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, R), roughness * radianceLevels).rgb * u_ReflectionStrength;
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
	float verticalDepth = fromBelow ? 1e6 : waterPath * max(V.y, 0.0);

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
	float edge = fromBelow ? 1.0 : clamp(verticalDepth / u_EdgeSoftness, 0.0, 1.0);
	color = vec4(mix(texture(u_SceneColor, screenUV).rgb, surface, edge), 1.0);

	// Shadows Only (Lights panel): the sun's shadow on the water, gray for the other lights (they don't light the water yet)
	if (u_ShadowDebug.x > 0.5)
	{
		color = vec4(vec3(u_ShadowDebug.x < 1.5 ? SunShadow(v_WorldPosition, up) : 0.2), 1.0);
	}
}
