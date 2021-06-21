struct PS_INPUT
{
	float4 position: SV_POSITION;
	float3 color: COLOR;
	float3 binormal: BINORMAL;
};

cbuffer constant: register(b0)
{
	unsigned int m_time;
}


float4 psmain(PS_INPUT input) : SV_TARGET
{
	return float4(lerp(input.color, input.binormal, (sin(m_time / 1000.0) + 1.0) / 2.0), 1.0);
}
