Texture2D Color: register(t0);
sampler ColorSampler: register(s0);

Texture2D Normal: register(t1);
sampler NormalSampler: register(s1);

struct PS_INPUT
{
	float4 position: SV_POSITION;
	float3 normal:   NORMAL0;
	float2 texcoord: TEXCOORD0;
	float3 direction_to_camera: DIRECTIONTOCAMERA0;
	row_major float3x3 tbn: TBN0;
};

cbuffer constant: register(b0)
{
	row_major float4x4 m_model;
	row_major float4x4 m_view;
	row_major float4x4 m_proj;
	float3 m_light_direction;
	float3 m_camera_position;
	float3 m_light_position;
	float m_light_radius;
	float m_time;
};


float4 psmain(PS_INPUT input) : SV_TARGET
{
	float4 colorSample = Color.Sample(ColorSampler, float2(input.texcoord.x, 1.0 - input.texcoord.y));

	return colorSample;
}
