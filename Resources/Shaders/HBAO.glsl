// ---------------------------------------
// -- Hazel Engine HBAO shader --
// ---------------------------------------

#type compute
#version 430

layout(std140, binding = 18) uniform HBAOData
{
	vec4	PerspectiveInfo;   // R = (x) * (R - L)/N ||| G = (y) * (T - B)/N ||| B =  L/N ||| A =  B/N
	vec2    InvQuarterResolution;
	float   RadiusToScreen;
	float   NegInvR2;
	float   NDotVBias;
	float   AOMultiplier;
	float   PowExponent;
	bool	IsOrtho;
	vec4    Float2Offsets[16];
	vec4    Jitters[16];
} u_Uniforms;

layout(binding = 0) uniform sampler2DArray u_LinearDepthTexArray;
layout(binding = 1) uniform sampler2D u_ViewNormalsTex;
layout(binding = 2) uniform sampler2D u_ViewPositionTex;
layout(binding = 19, rg16f) restrict writeonly uniform image2DArray o_Color;

// Unroll all loops for performance - this is important
#pragma optionNV(unroll all)

#define M_PI 3.14159265f
#define AO_RANDOMTEX_SIZE 4

const float  NUM_STEPS = 4;
const float  NUM_DIRECTIONS = 8; // texRandom/g_Jitter initialization depends on this

vec3 GetQuarterCoord(vec2 UV) {
	return vec3(UV, float(gl_GlobalInvocationID.z));
}

vec3 UVToView(vec2 uv, float eye_z)
{
	return vec3((uv * u_Uniforms.PerspectiveInfo.xy + u_Uniforms.PerspectiveInfo.zw) * (u_Uniforms.IsOrtho ? 1.0 : eye_z), eye_z);
}

vec3 FetchQuarterResViewPos(vec2 UV)
{
	float ViewDepth = textureLod(u_LinearDepthTexArray, GetQuarterCoord(UV), 0).x;
	return UVToView(UV, ViewDepth);
}

float Falloff(float distanceSquare)
{
	return distanceSquare * u_Uniforms.NegInvR2 + 1.0;
}

// P = view-space position at the kernel center
// N = view-space normal at the kernel center
// S = view-space position of the current sample
float ComputeAO(vec3 P, vec3 N, vec3 S)
{
	vec3 V = S - P;
	float VdotV = dot(V, V);
	float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);
	return clamp(NdotV - u_Uniforms.NDotVBias, 0, 1) * clamp(Falloff(VdotV), 0, 1);
}

vec2 RotateDirection(vec2 dir, vec2 cosSin)
{
	return vec2(dir.x * cosSin.x - dir.y * cosSin.y, dir.x * cosSin.y + dir.y * cosSin.x);
}

vec4 GetJitter()
{
	// Get the current jitter vector from the per-pass constant buffer
	return u_Uniforms.Jitters[gl_GlobalInvocationID.z];
}

float ComputeCoarseAO(vec2 fullResUV, float radiusPixels, vec4 rand, vec3 viewPosition, vec3 viewNormal)
{
	radiusPixels /= 4.0;

	// Divide by NUM_STEPS + 1 so that the furthest samples are not fully attenuated
	float stepSizePixels = radiusPixels / (NUM_STEPS + 1);

	const float alpha = 2.0 * M_PI / NUM_DIRECTIONS;
	float ao = 0;
	for (float directionIndex = 0; directionIndex < NUM_DIRECTIONS; ++directionIndex)
	{
		float angle = alpha * directionIndex;

		// Compute normalized 2D direction
		vec2 direction = RotateDirection(vec2(cos(angle), sin(angle)), rand.xy);

		// Jitter starting sample within the first step
		float rayPixels = (rand.z * stepSizePixels + 1.0);

		for (float stepIndex = 0; stepIndex < NUM_STEPS; ++stepIndex)
		{
			vec2 snappedUV = round(rayPixels * direction) * u_Uniforms.InvQuarterResolution + fullResUV;
			vec3 S = FetchQuarterResViewPos(snappedUV);
			rayPixels += stepSizePixels;

			ao += ComputeAO(viewPosition, viewNormal, S);
		}
	}
	ao *= u_Uniforms.AOMultiplier / (NUM_DIRECTIONS * NUM_STEPS);
	return clamp(1.0 - ao * 2.0, 0, 1);
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	vec2 float2Offset = u_Uniforms.Float2Offsets[gl_GlobalInvocationID.z].xy;
	vec2 base = gl_GlobalInvocationID.xy * 4 + float2Offset;
	vec2 uv = base * u_Uniforms.InvQuarterResolution / 4;

	vec3 viewPosition = texelFetch(u_ViewPositionTex, ivec2(base), 0).xyz;
	viewPosition.z = -viewPosition.z;
	vec3 normalAndAO = texelFetch(u_ViewNormalsTex, ivec2(base), 0).xyz;
	vec3 viewNormal = -(normalAndAO.xyz * 2.0 - 1.0);

	// Compute projection of disk of radius control.R into screen space
	float radiusPixels = u_Uniforms.RadiusToScreen / (u_Uniforms.IsOrtho ? 1.0 : viewPosition.z);

	// Get jitter vector for the current full-res pixel
	vec4 rand = GetJitter();

	float AO = ComputeCoarseAO(uv, radiusPixels, rand, viewPosition, viewNormal);

	imageStore(o_Color, ivec3(gl_GlobalInvocationID), vec4(pow(AO, u_Uniforms.PowExponent), viewPosition.z, 0.0, 1.0));
}


