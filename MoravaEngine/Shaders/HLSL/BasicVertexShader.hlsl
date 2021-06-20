float4 vsmain(float4 position: POSITION) : SV_POSITION
{
	position *= 1.8f;

	return position;
}
