struct PS_INPUT
{
	float4 position: SV_POSITION;
	float3 color: COLOR;
	float3 binormal: BINORMAL;
};

cbuffer constant: register(b0)
{
	row_major float4x4 model;
	row_major float4x4 view;
	row_major float4x4 projection;
	unsigned int time;
}


float4 psmain(PS_INPUT input) : SV_TARGET
{
	return float4(lerp(input.color, input.binormal, (sin(time / 1000.0) + 1.0) / 2.0), 1.0);
}
