struct VS_INPUT
{
	float4 position: POSITION0;
	float3 normal:   NORMAL0;
	float3 tangent:  TANGENT0;
	float3 binormal: BINORMAL0;
	float2 texcoord: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position: SV_POSITION;
	float2 texcoord: TEXCOORD0;
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
}


VS_OUTPUT vsmain(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	// World space
	output.position = mul(input.position, m_model);
	// View space
	output.position = mul(output.position, m_view);
	// Screen space
	output.position = mul(output.position, m_proj);

	output.texcoord = input.texcoord;

	return output;
}
