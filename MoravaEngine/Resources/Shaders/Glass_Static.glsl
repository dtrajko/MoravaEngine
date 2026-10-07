// Glass (SceneEnvMapVulkan): a material with Surface = Glass (see EnvMapVulkanMaterial::Surface). Light passes through it:
// what's behind is read from a copy of the scene made right before the glass is drawn (EnvMapVulkanGlass::CopyScene), at
// the place the refraction moves it to; the copy's smaller mip levels give rough glass its blur (frosted glass). On top:
// the environment's reflection and the lights' highlights, weighted by the Fresnel factor of the glass's index of
// refraction (more reflection at grazing angles, more transmission head-on).
//
// Descriptor sets: set 0 per frame and set 1 per material, declared as in the PBR shaders (Include/PBRCommon.glslh), so
// the same descriptor sets are bound; set 2: the scene copy.
// Push constants: the transform (vertex stage) and the glass values (fragment stage, GlassPushConstants in EnvMapVulkanGlass.h).
//
// Limits: the copy has only what was drawn before the glass (the opaque meshes, the skybox, the water), so glass behind
// glass isn't seen through it; and only what's on screen can be seen through it (near the edges, the unbent view is used).

#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec3 a_Tangent;
layout(location = 3) in vec3 a_Binormal;
layout(location = 4) in vec2 a_TexCoord;

#include "Include/FrameCamera.glslh"

layout (push_constant) uniform Transform
{
	mat4 u_Transform;
};

struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout (location = 0) out VertexOutput Output;
// The camera's view-projection for the fragment stage, which projects the refracted point onto the screen. Passed on
// rather than declared there: set 0's Camera block is declared in the vertex stage only, and must stay so (the per-frame
// descriptor set is shared with the PBR shaders, whose set 0 layout has it in the vertex stage).
layout (location = 10) flat out mat4 v_ViewProjection;

void main()
{
	Output.WorldPosition = vec3(u_Transform * vec4(a_Position, 1.0));
	Output.Normal = mat3(u_Transform) * a_Normal;
	Output.TexCoord = a_TexCoord;
	Output.WorldNormals = mat3(u_Transform) * mat3(a_Tangent, a_Binormal, a_Normal);
	Output.WorldTransform = mat3(u_Transform);
	Output.Binormal = a_Binormal;
	v_ViewProjection = u_ViewProjectionMatrix;

	gl_Position = u_ViewProjectionMatrix * u_Transform * vec4(a_Position, 1.0);
}

// ---------------------------------------------------------------------------------------------------

#type fragment
#version 450 core

#include "Include/PBRCommon.glslh"

layout (location = 10) flat in mat4 v_ViewProjection;

// Set 2: the scene as it was before the glass (color with a mip chain, depth)
layout (set = 2, binding = 0) uniform sampler2D u_SceneColor;
layout (set = 2, binding = 1) uniform sampler2D u_SceneDepth;

layout (push_constant) uniform GlassMaterial
{
	layout (offset = 64) vec3 TintColor; // the albedo color: the color of the light that passes through
	float IOR;                // index of refraction
	float Roughness;          // 0: clear, 1: fully frosted
	float Thickness;          // meters the light travels inside the glass
	float AlbedoTexToggle;
	float NormalTexToggle;
	float RoughnessTexToggle;
	float TilingFactor;
	float EmissiveTexToggle;
	float EmissiveIntensity;
	float MetalRoughPacked;
	float SceneCopyLevels;    // mip levels of u_SceneColor
	float Solid;              // 1: a solid body (bent entering and leaving, as a lens); 0: thin (a pane, a shell)
} u_Glass;

void main()
{
	float tiling = u_Glass.TilingFactor > 0.0 ? u_Glass.TilingFactor : 1.0;
	vec2 texCoord = Input.TexCoord * tiling;

	vec3 tint = u_Glass.AlbedoTexToggle > 0.5 ? texture(u_AlbedoTexture, texCoord).rgb : u_Glass.TintColor;
	float roughness = u_Glass.RoughnessTexToggle > 0.5
		? (u_Glass.MetalRoughPacked > 0.5 ? texture(u_RoughnessTexture, texCoord).g : texture(u_RoughnessTexture, texCoord).r)
		: u_Glass.Roughness;
	roughness = clamp(roughness, 0.0, 1.0);

	vec3 N = normalize(Input.Normal);
	if (u_Glass.NormalTexToggle > 0.5)
	{
		N = normalize(Input.WorldNormals * normalize(2.0 * texture(u_NormalTexture, texCoord).rgb - 1.0));
	}
	vec3 V = normalize(u_CameraPosition - Input.WorldPosition);
	if (dot(N, V) < 0.0)
	{
		N = -N; // seen from inside (back faces aren't drawn, but a normal map can tilt the normal away)
	}
	float NdotV = max(dot(N, V), 0.0);

	// The PBR lighting code (Include/PBRCommon.glslh) with no diffuse part: glass reflects, it doesn't scatter light back
	m_Params.Albedo = vec3(0.0);
	m_Params.Metalness = 0.0;
	m_Params.Roughness = max(roughness, 0.05); // the highlights' minimum, as in the PBR shaders
	m_Params.Normal = N;
	m_Params.View = V;
	m_Params.NdotV = NdotV;

	// Reflectance head-on, from the index of refraction: 4% for glass (1.5), 2% for water (1.33)
	float ior = max(u_Glass.IOR, 1.0);
	float f0 = (ior - 1.0) / (ior + 1.0);
	vec3 F0 = vec3(f0 * f0);
	vec3 F = fresnelSchlickRoughness(F0, NdotV, roughness);

	// Refraction. Thin glass (a pane, a shell, a hollow vase): the view ray bends where it enters, travels Thickness inside
	// and leaves through a parallel side, so it goes on in its old direction, only shifted a little.
	// Solid glass, through both sides: the view ray bends where it enters (this surface), travels Thickness inside, and
	// bends again where it leaves through the far side. The far side isn't drawn, so its normal is taken from a
	// ball: there the two normals of a chord are mirror images across the plane across the chord (the ray inside), so the
	// ray leaves at the angle it entered at and the bending doubles, as in a lens (head-on: none; toward the outline: a
	// lot). The ray then goes on to what's behind: the scene's depth at this pixel says how far that is, and the farther,
	// the more the bending moves it on the screen (a glass object shows the room behind it squeezed and turned around).
	// Where the ray can't leave anyway (total internal reflection: a normal map, numbers at grazing angles), the light
	// seen there was reflected inside the glass: the environment in that direction stands in for it.
	vec2 screenSize = vec2(textureSize(u_SceneColor, 0));
	vec2 screenUV = gl_FragCoord.xy / screenSize;
	vec2 refractedUV = screenUV;
	bool internalReflection = false;
	vec3 internalDirection = vec3(0.0);
	vec3 T = refract(-V, N, 1.0 / ior);
	if (dot(T, T) > 0.0 && ior > 1.0001)
	{
		vec3 exitPoint = Input.WorldPosition + T * u_Glass.Thickness;
		vec3 farNormal = N - 2.0 * dot(N, T) * T; // the far side's outward normal (see above)
		vec3 D = u_Glass.Solid > 0.5 ? refract(T, -farNormal, ior) : -V;
		if (dot(D, D) == 0.0)
		{
			internalReflection = true;
			internalDirection = reflect(T, -farNormal);
		}
		else
		{
			// How far behind the glass the scene is at this pixel (its depth, unprojected), at least past the glass
			mat4 inverseViewProjection = inverse(v_ViewProjection);
			float sceneDepth = texture(u_SceneDepth, screenUV).r;
			vec4 behindPoint = inverseViewProjection * vec4(screenUV * 2.0 - 1.0, sceneDepth, 1.0);
			float behindDistance = sceneDepth < 1.0 ? length(behindPoint.xyz / behindPoint.w - Input.WorldPosition) : 20.0; // the sky: far
			float travel = max(behindDistance - u_Glass.Thickness, 0.0);

			vec4 surfaceClip = v_ViewProjection * vec4(Input.WorldPosition, 1.0);
			vec4 targetClip = v_ViewProjection * vec4(exitPoint + D * travel, 1.0);
			if (targetClip.w > 0.0)
			{
				// NDC to texture coordinates: both run the same way as the framebuffer (half the range, same direction)
				refractedUV = screenUV + (targetClip.xy / targetClip.w - surfaceClip.xy / surfaceClip.w) * 0.5;
			}
		}
	}
	// Off the screen, or onto something in front of the glass (nearer than this surface): not seen through it, so the
	// unbent view is used there
	if (any(lessThan(refractedUV, vec2(0.0))) || any(greaterThan(refractedUV, vec2(1.0))) ||
		texture(u_SceneDepth, refractedUV).r < gl_FragCoord.z)
	{
		refractedUV = screenUV;
	}

	// Rough glass: a smaller (blurrier) level of the scene copy. And where the refraction squeezes what's behind (a lens
	// shrinks the room behind it), the level that matches how far the lookup moves from pixel to pixel, as mipmapping
	// picks a texture's level: full detail there would sparkle (aliasing)
	vec2 texel = refractedUV * screenSize;
	float footprint = max(length(dFdx(texel)), length(dFdy(texel)));
	float aliasLevel = log2(max(footprint, 1.0));
	float blurLevel = min(max(roughness * min(u_Glass.SceneCopyLevels - 1.0, 6.0), aliasLevel), u_Glass.SceneCopyLevels - 1.0);
	vec3 behind = textureLod(u_SceneColor, refractedUV, blurLevel).rgb;
	if (internalReflection)
	{
		int levels = textureQueryLevels(u_EnvRadianceTex);
		behind = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, internalDirection), roughness * levels).rgb *
			WaterSkylightAt(Input.WorldPosition);
	}
	vec3 transmitted = behind * tint;

	// The environment's reflection (prefiltered by roughness, as in IBL) and the lights' highlights
	vec3 R = reflect(-V, N);
	int radianceLevels = textureQueryLevels(u_EnvRadianceTex);
	vec3 environment = textureLod(u_EnvRadianceTex, RotateVectorAboutY(u_EnvMapRotation, R), roughness * radianceLevels).rgb;
	vec2 brdf = texture(u_BRDFLUTTexture, vec2(NdotV, 1.0 - roughness)).rg;
	vec3 reflected = environment * (F0 * brdf.x + brdf.y) * WaterSkylightAt(Input.WorldPosition);
	vec3 highlights = Lighting(F0);

	vec3 emissive = u_Glass.EmissiveTexToggle > 0.5 ? texture(u_EmissiveTexture, texCoord).rgb * u_Glass.EmissiveIntensity : vec3(0.0);

	color = vec4(transmitted * (1.0 - F) + reflected + highlights + emissive, 1.0);

	if (u_ShowCascades > 0.5 && u_ShadowsEnabled > 0.5)
	{
		color.rgb *= CascadeDebugColor(Input.WorldPosition);
	}
	if (u_ShadowDebug.x > 0.5)
	{
		color = vec4(vec3(ShadowDebugValue()), 1.0);
	}
}
