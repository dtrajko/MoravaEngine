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
	float3 binormal: BINORMAL0;
};

cbuffer constant: register(b0)
{
	row_major float4x4 model;
	row_major float4x4 view;
	row_major float4x4 projection;
	unsigned int time;
}


VS_OUTPUT vsmain(VS_INPUT input)
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	// World space
	output.position = mul(input.position, model);
	// View space
	output.position = mul(output.position, view);
	// Screen space
	output.position = mul(output.position, projection);

	output.texcoord = input.texcoord;
	output.binormal = input.binormal;

	return output;
}
